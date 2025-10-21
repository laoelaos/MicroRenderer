//
// Created by laoe on 25-9-6.
//

#include <algorithm>
#include <iostream>

#include "Rasterizer.h"

#include "ImageUtils.h"
#include "Texture.h"
#include "Logger.h"

Rasterizer::Rasterizer() {
    m_width = 1;
    m_height = 1;
    m_model = identity_matrix<4>();
    m_view = identity_matrix<4>();
    m_projection = identity_matrix<4>();
    m_Viewport = identity_matrix<4>();
    m_zBuffer = {};
    m_colorBuffer = {};
}

Rasterizer& Rasterizer::get() {
    static Rasterizer instance;
    return instance;
}

void Rasterizer::set_msaa(int MSAA) {
    m_MSAA = MSAA;
}

void Rasterizer::pass(const Scene& scene, RasterizerMode mode, FrameBuffer& frame_buffer) {
    m_finalColorBuffer = frame_buffer.colorBuffer;
    m_finalZBuffer = frame_buffer.zBuffer;
    m_width = scene.camera.getWidth() * m_MSAA;
    m_height = scene.camera.getHeight() * m_MSAA;

    if (m_width <= 0 || m_height <= 0) {
        LOGE("Rasterizer::pass: Invalid render dimensions: {}x{}", m_width, m_height);
        return;
    }

    build_buffer();
    m_view = scene.camera.get_view_matrix();
    m_projection = scene.camera.get_projection_matrix();
    m_Viewport = scene.camera.get_viewport_matrix(m_MSAA);

    if (mode == RasterizerMode_SKYBOX) {
        SkyboxPipeline(scene);
        mode = RasterizerMode_PHONG_SHADOW;
    }

    if (mode == RasterizerMode_PHONG || mode == RasterizerMode_PHONG_SHADOW) {
        PhongShader::s_lightPos = {};
        for (const Light& light : scene.lights) {
            PhongShader::s_lightPos.push_back((m_view * light.getPosition().to_vec4(1.0)).to_vec3_point());
        }
    }

    for (const Model& obj_model: scene.models) {

        if (!obj_model.enable)
            continue;

        m_model = obj_model.mesh->GetTransformMatrix();
        m_MVP = m_projection * m_view * m_model;
        m_MV = m_view * m_model;
        m_MVit = (m_view * m_model).invert().transpose();

        if (mode == RasterizerMode_ZTEST)
            ZtestPipeline(obj_model);
        if (mode == RasterizerMode_PHONG) {
            PhongShader::s_EnableShadow = false;
            ZtestPipeline(obj_model);
            PhongPipeline(obj_model);
        }
        if (mode == RasterizerMode_PHONG_SHADOW) {
            PhongShader::s_lightInfo = &scene.lights;
            PhongShader::s_mainCameraM = m_view.invert();
            PhongShader::s_EnableShadow = true;
            ZtestPipeline(obj_model);
            PhongPipeline(obj_model);
        }
    }

    FillInColor();
    FillInZVal();
}

void Rasterizer::PhongPipeline(const Model& model) {
    Mesh mesh = *model.mesh;
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
#pragma omp parallel for default(none) shared(material, mesh, diffuse_mapping, normal_type, shade_frequency, tri_count, m_width, m_height, m_colorBuffer, m_zBuffer, m_tileLocks)
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
                if (z < m_zBuffer.Get(x, y)) {
                    continue;
                }
                m_zBuffer.Set(x, y, z);

                shader.viewWorldPos = tri.get_interpolated_world_position();
                shader.tex_coords = tri.get_interpolated_tex_coords();
                shader.color = diffuse_mapping ? ImageUtil::RGBAtoVec3(material.texture->Get(shader.tex_coords)) : vec3{0.5, 0.5, 0.5};

                if (normal_type == NormalMapType_GLOBAL) {
                    shader.normal = ImageUtil::RGBAtoVec3(material.normal_map->Get(shader.tex_coords)) * 2 - vec3{1, 1, 1};
                } else if (shade_frequency == ShadeFrequency_PER_FRAGMENT) {
                    shader.normal = tri.get_interpolated_normal();
                } else {
                    shader.normal = normalize((tri.world_vertices[1] - tri.world_vertices[0]) ^
                                               (tri.world_vertices[2] - tri.world_vertices[1]));
                }
                if (normal_type == NormalMapType_TANGENT) {
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
                            TBN * (ImageUtil::RGBAtoVec3(material.normal_map->Get(shader.tex_coords)) * 2 - vec3{1, 1, 1}));
                    }
                }

                m_colorBuffer.Set(x, y, shader.shade());
            }
        }
    }
}

void Rasterizer::ZtestPipeline(const Model& model) {
    Mesh mesh = *model.mesh;
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
                //TODO:注释后光源位于物体内不会报错,待解决
                //std::lock_guard guard(m_tileLocks[get_tile_lock(x, y)]);
                double z = tri.get_interpolated_z();
                if (z >= m_zBuffer.Get(x, y)) {
                    m_zBuffer.Set(x, y, z);
                }
            }
        }
    }
}

void Rasterizer::SkyboxPipeline(const Scene &scene) {
    if (!scene.skybox_texture) return;
    m_MVPVi = (m_Viewport * m_projection * m_view).invert();
    SkyboxFragment(scene.skybox_texture);
}

void Rasterizer::SkyboxFragment(const std::shared_ptr<SingleCubeTexture>& skybox_texture) {
#pragma omp parallel for default(none) shared(skybox_texture, m_width, m_height, m_colorBuffer, m_zBuffer, m_tileLocks)
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            std::lock_guard guard(m_tileLocks[get_tile_lock(x, y)]);
            if (m_zBuffer.Get(x, y) > -std::numeric_limits<double>::infinity()) {
                continue;
            }

            vec4 screen_pos = {(double)x + 0.5, (double)y + 0.5, -1.0, 1.0};
            vec4 world_pos = m_MVPVi * screen_pos;
            vec3 dir = normalize(world_pos.to_vec3_point());

            m_colorBuffer.Set(x, y, ImageUtil::RGBAtoVec3(skybox_texture->Get(dir)));
        }
    }
}

void Rasterizer::FillInColor() {
    if (!m_finalColorBuffer)
        return;

    int final_w = m_finalColorBuffer->GetWidth();
    int final_h = m_finalColorBuffer->GetHeight();
#pragma omp parallel for default(none) shared(final_w, final_h)
    for (int y = 0; y < final_h; y++) {
        for (int x = 0; x < final_w; x++) {
            vec3 color_sum;
            for (int dy = 0; dy < m_MSAA; dy++) {
                for (int dx = 0; dx < m_MSAA; dx++) {
                    color_sum += m_colorBuffer.Get(x * m_MSAA + dx, y * m_MSAA + dy);
                }
            }
            m_finalColorBuffer->Set(x, y, ImageUtil::Vec3ToRGBA(color_sum / (m_MSAA * m_MSAA)));
        }
    }
}
//TODO: framebuffer,zbuffer与camera未必同尺寸，待修改
void Rasterizer::FillInZVal() {
    if (!m_finalZBuffer)
        return;

    int final_w = m_finalZBuffer->GetWidth();
    int final_h = m_finalZBuffer->GetHeight();
#pragma omp parallel for default(none) shared(final_w, final_h)
    for (int y = 0; y < final_h; y++) {
        for (int x = 0; x < final_w; x++) {
            double max_z = -std::numeric_limits<double>::infinity();
            for (int dy = 0; dy < m_MSAA; dy++) {
                for (int dx = 0; dx < m_MSAA; dx++) {
                    max_z = std::max(max_z, m_zBuffer.Get(x * m_MSAA + dx, y * m_MSAA + dy));
                }
            }
            m_finalZBuffer->Set(x, y, ImageUtil::EncodeZ(max_z));
        }
    }
}

void Rasterizer::build_buffer() {
    if (m_width <= 0 || m_height <= 0) {
        LOGE("Rasterizer::build_buffer: Invalid buffer dimensions: {}x{}", m_width, m_height);
        return;
    }
    m_zBuffer = Buffer<double>(m_width, m_height);
    m_zBuffer.SetAll( -std::numeric_limits<double>::infinity());
    m_colorBuffer = Buffer<vec3>(m_width, m_height);
    m_colorBuffer.SetAll(vec3());
}
