//
// Created by laoe on 25-9-6.
//

#include <algorithm>
#include <iostream>

#include "Rasterizer.h"
#include "Model.h"
#include "Shader.h"

Rasterizer::Rasterizer() {
    m_width = 1;
    m_height = 1;
    m_model = identity_matrix<4>();
    m_view = identity_matrix<4>();
    m_projection = identity_matrix<4>();
    m_viewport = identity_matrix<4>();
    m_lights = {};
    m_texture = {};
    m_normalMap = {};
    m_mode = PHONG_WITH_SHADOW;
    build_buffer();
}

void Rasterizer::build_buffer() {
    m_zBuffer.resize(m_width * m_height);
    m_frameBuffer.resize(m_width * m_height);
    std::ranges::fill(m_frameBuffer, vec3());
    std::ranges::fill(m_zBuffer, -std::numeric_limits<double>::infinity());
}

//TODO: 暂时禁用MSAA, 目前实现方式不好
void Rasterizer::set_msaa(int MSAA) {
    // double change_msaa = static_cast<double>(MSAA) / this->MSAA;
    // this->MSAA = MSAA;
    // height *= change_msaa;
    // width  *= change_msaa;
    // z_buffer.resize(width * height);
    // framebuffer.resize(width * height);
    // clear_buffer();
}

//TODO: 光栅化管线
//TODO: 目前透视投影前z=0的点经过透视后w=0, 导致无法投影到屏幕上，应该实现裁剪来处理

void Rasterizer::rasterize(Scene &scene) {
    if (m_mode == ZTEST) {
        pass(scene, ZTEST);
    } else if (m_mode == PHONG) {
        pass(scene, PHONG);
        Phong_Shadow_Shader::camera_pos = scene.camera.center;
    } else if (m_mode == PHONG_WITH_SHADOW) {
        if (scene.light_move) {
            Camera tmp = scene.camera;
            Phong_Shadow_Shader::LightMaps.clear();
            Phong_Shadow_Shader::LightN.clear();

            for (Light& light : scene.lights) {
                if (light.LightCamera.has_value())
                    scene.camera = light.LightCamera.value();
                else
                    throw std::runtime_error("one light dont have a light camera");

                pass(scene, ZTEST);

                auto light_map = TGAImage(scene.camera.width, scene.camera.height, TGAImage::BIG_GRAYSCALE);
                zbuffer_to_TGA(light_map);
                Phong_Shadow_Shader::LightMaps.push_back(light_map);
                Phong_Shadow_Shader::LightN.push_back(light.LightCamera->get_viewport_matrix() * light.LightCamera->get_projection_matrix() * light.LightCamera->get_view_matrix());
            }

            scene.light_move = false;
            scene.camera = tmp;
        }
        Phong_Shadow_Shader::camera_pos = scene.camera.center;
        pass(scene, PHONG_WITH_SHADOW);
    }
}

void Rasterizer::pass(const Scene& scene, RasterizerMode mode) {
    m_width = scene.camera.width;
    m_height = scene.camera.height;
    build_buffer();
    m_view = scene.camera.get_view_matrix();
    m_projection = scene.camera.get_projection_matrix();
    m_viewport = scene.camera.get_viewport_matrix();
    m_lights = scene.lights;
    for (const Model& obj_model: scene.models) {
        m_model = obj_model.get_transform_matrix();
        m_MVPV = m_viewport * m_projection * m_view * m_model;
        m_MV = m_view * m_model;
        m_MVit = (m_view * m_model).invert().transpose();

        if (mode == ZTEST)
            Ztest(obj_model);
        if (mode == PHONG_WITH_SHADOW) {
            Phong_Shadow_Shader::MainCameraM = m_view.invert();
            Ztest(obj_model);
            Phong(obj_model);
        }
        if (mode == PHONG) {
            Ztest(obj_model);
            Phong(obj_model);
        }
    }
}

void Rasterizer::Phong(const Model& model) {
    m_texture = model.material.texture;
    m_normalMap = model.material.normal_map;
    bool diffuse_mapping = model.material.diffuse_mapping;
    NormalType normal_type = model.material.normal_type;
    ShadeFrequency shade_frequency = model.material.shade_frequency;

    size_t tri_count = model.triangles.size();
#pragma omp parallel for default(none) shared(model, tri_count, diffuse_mapping, normal_type, shade_frequency, m_MVit, m_MVPV, m_MV, m_width, m_height, m_frameBuffer, m_zBuffer, m_lights, m_texture, m_normalMap)
    for (size_t idx = 0; idx < tri_count; ++idx) {
        Phong_Shadow_Shader shader;
        shader.light_info = m_lights;
        shader.properties = &model.material.properties;

        triangle now_triangle = model.triangles[idx];
        now_triangle.get_vertices(m_MVPV, m_MV);
        now_triangle.get_normal(m_MVit);
        if (now_triangle.is_backface())
             continue;

        auto [x_min, x_max, y_min, y_max] = now_triangle.find_bounding_box_int(m_width, m_height);
        for (int x = x_min; x <= x_max; x++) {
            for (int y = y_min; y <= y_max; y++) {
                now_triangle.get_barycentric_correct(x + .5, y + .5);
                if (now_triangle.is_invalid())
                     continue;

                std::lock_guard guard(m_tileLocks[get_tile_lock(x, y)]);
                double z = now_triangle.get_interpolated_z();
                if (z >= m_zBuffer[get_index(x, y)]) {
                    m_zBuffer[get_index(x, y)] = z;

                    shader.position = now_triangle.get_interpolated_world_position();
                    shader.tex_coords = now_triangle.get_interpolated_tex_coords();

                    if (diffuse_mapping) {
                        shader.color = m_texture.get_bilinear(shader.tex_coords);
                    } else {
                        shader.color = {0.5, 0.5, 0.5};
                    }

                    if (normal_type == GLOBAL) {
                        shader.normal = m_normalMap.get_bilinear(shader.tex_coords) * 2 - vec3{1, 1, 1};
                    } else if (shade_frequency == PER_FRAGMENT) {
                        shader.normal = now_triangle.get_interpolated_normal();
                    } else {
                        shader.normal = normalize((now_triangle.world_vertices[1] - now_triangle.world_vertices[0]) ^
                                                   (now_triangle.world_vertices[2] - now_triangle.world_vertices[1]));
                    }
                    if (normal_type == TANGENT) {
                        vec3 e1 = now_triangle.world_vertices[1] - now_triangle.world_vertices[0];
                        vec3 e2 = now_triangle.world_vertices[2] - now_triangle.world_vertices[0];
                        vec2 delta_uv1 = now_triangle.tex_coords[1] - now_triangle.tex_coords[0];
                        vec2 delta_uv2 = now_triangle.tex_coords[2] - now_triangle.tex_coords[0];
                        mat<2, 2> uv_matrix{{{delta_uv1.x, delta_uv2.x}, {delta_uv1.y, delta_uv2.y}}};
                        mat<3, 2> edge_matrix{{{e1.x, e2.x}, {e1.y, e2.y}, {e1.z, e2.z}}};
                        if (std::abs(determinant(uv_matrix)) > 1e-8) {
                            mat<3, 2> tnb = edge_matrix * uv_matrix.invert();
                            vec3 t = normalize(tnb.get_col(0));
                            vec3 b = normalize(tnb.get_col(1));
                            vec3 n = shader.normal;
                            mat<3, 3> TBN{{{t.x, b.x, n.x}, {t.y, b.y, n.y}, {t.z, b.z, n.z}}};
                            shader.normal = normalize(
                                TBN * (m_normalMap.get_bilinear(shader.tex_coords) * 2 - vec3{1, 1, 1}));
                        }
                    }

                    m_frameBuffer[get_index(x, y)] = shader.shade();
                }
            }
        }
    }
}

void Rasterizer::Ztest(const Model& model) {
#pragma omp parallel for default(none) shared(model, m_MVPV, m_MV, m_width, m_height, m_zBuffer, m_tileLocks)
    for (auto now_triangle : model.triangles) {
        now_triangle.get_vertices(m_MVPV, m_MV);
        if (now_triangle.is_backface())
            continue;

        auto [x_min, x_max, y_min, y_max] = now_triangle.find_bounding_box_int(m_width, m_height);
        for (int x = x_min; x <= x_max; x++) {
            for (int y = y_min; y <= y_max; y++) {
                now_triangle.get_barycentric(x + .5, y + .5);
                if (now_triangle.is_invalid())
                    continue;

                std::lock_guard guard(m_tileLocks[get_tile_lock(x, y)]);
                double z = now_triangle.get_interpolated_z();
                if (z >= m_zBuffer[get_index(x, y)]) {
                    m_zBuffer[get_index(x, y)] = z;
                }
            }
        }
    }
}

void Rasterizer::framebuffer_to_TGA(TGAImage& framebuffer_) {
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            framebuffer_.set(x, y, TGAColor(m_frameBuffer[get_index(x, y)]));
        }
    }
}

void Rasterizer::zbuffer_to_TGA(TGAImage& framebuffer_) {
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            framebuffer_.set(x, y, TGAColor(m_zBuffer[get_index(x, y)]));
        }
    }
}
