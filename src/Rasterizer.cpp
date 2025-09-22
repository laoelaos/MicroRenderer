//
// Created by laoe on 25-9-6.
//

#include <algorithm>
#include <iostream>

#include "Rasterizer.h"
#include "Model.h"
#include "Shader.h"

Rasterizer::Rasterizer() {
    width = 1;
    height = 1;
    model = identity_matrix<4>();
    view = identity_matrix<4>();
    projection = identity_matrix<4>();
    viewport = identity_matrix<4>();
    lights = {};
    texture = {};
    normal_map = {};
    mode = PHONG_WITH_SHADOW;
    build_buffer();
}

void Rasterizer::build_buffer() {
    z_buffer.resize(width * height);
    framebuffer.resize(width * height);
    std::ranges::fill(framebuffer, vec3());
    std::ranges::fill(z_buffer, -std::numeric_limits<double>::infinity());
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
    if (mode == ZTEST) {
        pass(scene, ZTEST);
    } else if (mode == PHONG) {
        pass(scene, PHONG);
        Phong_Shadow_Shader::camera_pos = scene.camera.center;
    } else if (mode == PHONG_WITH_SHADOW) {
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
    width = scene.camera.width;
    height = scene.camera.height;
    build_buffer();
    view = scene.camera.get_view_matrix();
    projection = scene.camera.get_projection_matrix();
    viewport = scene.camera.get_viewport_matrix();
    lights = scene.lights;
    for (const Model& obj_model: scene.models) {
        model = obj_model.get_transform_matrix();
        mvpv = viewport * projection * view * model;
        mv = view * model;
        mvit = (view * model).invert().transpose();

        if (mode == ZTEST)
            Ztest(obj_model);
        if (mode == PHONG_WITH_SHADOW) {
            Phong_Shadow_Shader::MainCameraM = view.invert();
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
    texture = model.material.texture;
    normal_map = model.material.normal_map;
    bool diffuse_mapping = model.material.diffuse_mapping;
    NormalType normal_type = model.material.normal_type;
    ShadeFrequency shade_frequency = model.material.shade_frequency;

    size_t tri_count = model.triangles.size();
#pragma omp parallel for default(none) shared(model, tri_count, diffuse_mapping, normal_type, shade_frequency, mvit, mvpv, mv, width, height, framebuffer, z_buffer, lights, texture, normal_map)
    for (size_t idx = 0; idx < tri_count; ++idx) {
        Phong_Shadow_Shader shader;
        shader.light_info = lights;
        shader.properties = &model.material.properties;

        triangle now_triangle = model.triangles[idx];
        now_triangle.get_vertices(mvpv, mv);
        now_triangle.get_normal(mvit);
        if (now_triangle.is_backface())
             continue;

        auto [x_min, x_max, y_min, y_max] = now_triangle.find_bounding_box_int(width, height);
        for (int x = x_min; x <= x_max; x++) {
            for (int y = y_min; y <= y_max; y++) {
                now_triangle.get_barycentric_correct(x + .5, y + .5);
                if (now_triangle.is_invalid())
                     continue;

                std::lock_guard guard(tile_locks[get_tile_lock(x, y)]);
                double z = now_triangle.get_interpolated_z();
                if (z >= z_buffer[get_index(x, y)]) {
                    z_buffer[get_index(x, y)] = z;

                    shader.position = now_triangle.get_interpolated_world_position();
                    shader.tex_coords = now_triangle.get_interpolated_tex_coords();

                    if (diffuse_mapping) {
                        shader.color = texture.get_bilinear(shader.tex_coords);
                    } else {
                        shader.color = {0.5, 0.5, 0.5};
                    }

                    if (normal_type == GLOBAL) {
                        shader.normal = normal_map.get_bilinear(shader.tex_coords) * 2 - vec3{1, 1, 1};
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
                                TBN * (normal_map.get_bilinear(shader.tex_coords) * 2 - vec3{1, 1, 1}));
                        }
                    }

                    framebuffer[get_index(x, y)] = shader.shade();
                }
            }
        }
    }
}

void Rasterizer::Ztest(const Model& model) {
#pragma omp parallel for default(none) shared(model, mvpv, mv, width, height, z_buffer, tile_locks)
    for (auto now_triangle : model.triangles) {
        now_triangle.get_vertices(mvpv, mv);
        if (now_triangle.is_backface())
            continue;

        auto [x_min, x_max, y_min, y_max] = now_triangle.find_bounding_box_int(width, height);
        for (int x = x_min; x <= x_max; x++) {
            for (int y = y_min; y <= y_max; y++) {
                now_triangle.get_barycentric(x + .5, y + .5);
                if (now_triangle.is_invalid())
                    continue;

                std::lock_guard guard(tile_locks[get_tile_lock(x, y)]);
                double z = now_triangle.get_interpolated_z();
                if (z >= z_buffer[get_index(x, y)]) {
                    z_buffer[get_index(x, y)] = z;
                }
            }
        }
    }
}

void Rasterizer::framebuffer_to_TGA(TGAImage& framebuffer_) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            framebuffer_.set(x, y, TGAColor(framebuffer[get_index(x, y)]));
        }
    }
}

void Rasterizer::zbuffer_to_TGA(TGAImage& framebuffer_) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            framebuffer_.set(x, y, TGAColor(z_buffer[get_index(x, y)]));
        }
    }
}
