//
// Created by laoe on 25-9-6.
//

#include <algorithm>
#include <iostream>

#include "Rasterizer.h"


Rasterizer::Rasterizer() {
    m_width = 1;
    m_height = 1;
    m_model = identity_matrix<4>();
    m_view = identity_matrix<4>();
    m_projection = identity_matrix<4>();
    m_Viewport = identity_matrix<4>();
    build_buffer();
}

Rasterizer& Rasterizer::get() {
    static Rasterizer instance;
    return instance;
}

//TODO: 暂时禁用MSAA, 目前实现方式不好
void Rasterizer::set_msaa(int MSAA) {
    m_MSAA = MSAA;
}

void Rasterizer::rasterize(Scene &scene, RasterizerMode mode) {
    if (mode == ZTEST) {
        pass(scene, ZTEST);
    } else if (mode == PHONG) {
        pass(scene, PHONG);
    } else if (mode == PHONG_WITH_SHADOW) {
        for (Light& light : scene.lights) {
            light.ProcessShadowMapIfNeeded(scene);
        }
        pass(scene, PHONG_WITH_SHADOW);
    }
}

void Rasterizer::pass(const Scene& scene, RasterizerMode mode) {
    m_width = scene.camera.getWidth() * m_MSAA;
    m_height = scene.camera.getHeight() * m_MSAA;
    build_buffer();
    m_view = scene.camera.get_view_matrix();
    m_projection = scene.camera.get_projection_matrix();
    m_Viewport = scene.camera.get_viewport_matrix(m_MSAA);

    if (mode == PHONG || mode == PHONG_WITH_SHADOW) {
        PhongShader::s_lightPos = {};
        for (const Light& light : scene.lights) {
            PhongShader::s_lightPos.push_back((m_view * light.getPosition().to_vec4(1.0)).to_vec3_point());
        }
    }

    for (const Model& obj_model: scene.models) {

        if (!obj_model.enable)
            continue;

        m_model = obj_model.mesh.GetTransformMatrix();
        m_MVP = m_projection * m_view * m_model;
        m_MV = m_view * m_model;
        m_MVit = (m_view * m_model).invert().transpose();

        if (mode == ZTEST)
            ZtestPipeline(obj_model);
        if (mode == PHONG) {
            PhongShader::s_EnableShadow = false;
            ZtestPipeline(obj_model);
            PhongPipeline(obj_model);
        }
        if (mode == PHONG_WITH_SHADOW) {
            PhongShader::s_lightInfo = &scene.lights;
            PhongShader::s_mainCameraM = m_view.invert();
            PhongShader::s_EnableShadow = true;
            ZtestPipeline(obj_model);
            PhongPipeline(obj_model);
        }
    }
}

void Rasterizer::PhongPipeline(const Model& model) {
    Mesh mesh = model.mesh;
    mesh.ProcessTransform(m_MVP, m_MV, m_MVit);
    mesh.ProcessClipping();
    mesh.ProcessViewport(m_Viewport);
    mesh.ProcessFaceCulling();
    PhongFragment(model.material, mesh);
}

void Rasterizer::PhongFragment(const Material &material, Mesh &mesh) {
    bool diffuse_mapping = material.diffuse_mapping;
    NormalMapType normal_type = material.normal_type;
    ShadeFrequency shade_frequency = material.shade_frequency;

    size_t tri_count = mesh.triangles.size();
#pragma omp parallel for default(none) shared(material, mesh, diffuse_mapping, normal_type, shade_frequency, tri_count, m_width, m_height, m_frameBuffer, m_zBuffer, m_tileLocks)
    for (size_t idx = 0; idx < tri_count; ++idx) {
        Triangle& tri = mesh.triangles[idx];
        if (tri.discard)
            continue;

        PhongShader shader;
        shader.properties = &material.properties;


        auto [x_min, x_max, y_min, y_max] = tri.find_bounding_box_int(m_width, m_height);
        for (int x = x_min; x <= x_max; x++) {
            for (int y = y_min; y <= y_max; y++) {
                tri.get_barycentric_correct(x + .5, y + .5);
                if (tri.is_invalid())
                     continue;

                std::lock_guard guard(m_tileLocks[get_tile_lock(x, y)]);
                double z = tri.get_interpolated_z();
                if (z < m_zBuffer[get_index(x, y)]) {
                    continue;
                }
                m_zBuffer[get_index(x, y)] = z;

                shader.viewWorldPos = tri.get_interpolated_world_position();
                shader.tex_coords = tri.get_interpolated_tex_coords();
                shader.color = diffuse_mapping ? material.texture.get_bilinear(shader.tex_coords) : vec3{0.5, 0.5, 0.5};

                if (normal_type == GLOBAL) {
                    shader.normal = material.normal_map.get_bilinear(shader.tex_coords) * 2 - vec3{1, 1, 1};
                } else if (shade_frequency == PER_FRAGMENT) {
                    shader.normal = tri.get_interpolated_normal();
                } else {
                    shader.normal = normalize((tri.world_vertices[1] - tri.world_vertices[0]) ^
                                               (tri.world_vertices[2] - tri.world_vertices[1]));
                }
                if (normal_type == TANGENT) {
                    vec3 e1 = tri.world_vertices[1] - tri.world_vertices[0];
                    vec3 e2 = tri.world_vertices[2] - tri.world_vertices[0];
                    vec2 delta_uv1 = tri.tex_coords[1] - tri.tex_coords[0];
                    vec2 delta_uv2 = tri.tex_coords[2] - tri.tex_coords[0];
                    mat<2, 2> uv_matrix{{{delta_uv1.x, delta_uv2.x}, {delta_uv1.y, delta_uv2.y}}};
                    mat<3, 2> edge_matrix{{{e1.x, e2.x}, {e1.y, e2.y}, {e1.z, e2.z}}};
                    if (std::abs(determinant(uv_matrix)) > 1e-8) {
                        mat<3, 2> tnb = edge_matrix * uv_matrix.invert();
                        vec3 t = normalize(tnb.get_col(0));
                        vec3 b = normalize(tnb.get_col(1));
                        vec3 n = shader.normal;
                        mat<3, 3> TBN{{{t.x, b.x, n.x}, {t.y, b.y, n.y}, {t.z, b.z, n.z}}};
                        shader.normal = normalize(
                            TBN * (material.normal_map.get_bilinear(shader.tex_coords) * 2 - vec3{1, 1, 1}));
                    }
                }

                m_frameBuffer[get_index(x, y)] = shader.shade();
            }
        }
    }
}

void Rasterizer::ZtestPipeline(const Model& model) {
    Mesh mesh = model.mesh;
    mesh.ProcessTransform(m_MVP, m_MV, m_MVit);
    mesh.ProcessClipping();
    mesh.ProcessViewport(m_Viewport);
    mesh.ProcessFaceCulling();
    ZtestFragment(mesh);
}

void Rasterizer::ZtestFragment(Mesh &mesh) {
    size_t tri_count = mesh.triangles.size();
#pragma omp parallel for default(none) shared(mesh, tri_count, m_width, m_height, m_zBuffer, m_tileLocks)
    for (size_t idx = 0; idx < tri_count; ++idx) {
        Triangle& tri = mesh.triangles[idx];
        if (tri.discard)
            continue;

        auto [x_min, x_max, y_min, y_max] = tri.find_bounding_box_int(m_width, m_height);
        for (int x = x_min; x <= x_max; x++) {
            for (int y = y_min; y <= y_max; y++) {
                tri.get_barycentric(x + .5, y + .5);
                if (tri.is_invalid())
                    continue;

                std::lock_guard guard(m_tileLocks[get_tile_lock(x, y)]);
                double z = tri.get_interpolated_z();
                if (z >= m_zBuffer[get_index(x, y)]) {
                    m_zBuffer[get_index(x, y)] = z;
                }
            }
        }
    }
}

void Rasterizer::build_buffer() {
    m_zBuffer.resize(m_width * m_height);
    m_frameBuffer.resize(m_width * m_height);
    std::ranges::fill(m_frameBuffer, vec3());
    std::ranges::fill(m_zBuffer, -std::numeric_limits<double>::infinity());
}

void Rasterizer::framebuffer_to_TGA(TGAImage& framebuffer_) {
    for (int y = 0; y < m_height; y+=m_MSAA) {
        for (int x = 0; x < m_width; x+=m_MSAA) {
            vec3 color_sum;
            for (int dy = 0; dy < m_MSAA; dy++) {
                for (int dx = 0; dx < m_MSAA; dx++) {
                    color_sum += m_frameBuffer[get_index(x + dx, y + dy)];
                }
            }
            framebuffer_.set(x / m_MSAA, y / m_MSAA, TGAColor(color_sum / (m_MSAA * m_MSAA)));
        }
    }
}

void Rasterizer::zBuffer_to_TGA(TGAImage& framebuffer_) {
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            framebuffer_.set(x, y, TGAColor(m_zBuffer[get_index(x, y)]));
        }
    }
}
