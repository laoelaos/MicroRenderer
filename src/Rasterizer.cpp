//
// Created by laoe on 25-9-6.
//

#include <algorithm>
#include <iostream>

#include "Rasterizer.h"
#include "Model.h"

Rasterizer::Rasterizer(int w, int h) : width(w), height(h) {
    z_buffer.resize(w * h);
    framebuffer.resize(w * h);
    model = identity_matrix<4>();
    view = identity_matrix<4>();
    projection = identity_matrix<4>();
    viewport = identity_matrix<4>();
    lights = {};
    texture = {};
    normal_map = {};
    fragment_shader = nullptr;
    clear_buffer();
}

void Rasterizer::clear_buffer() {
    std::ranges::fill(framebuffer, vec3());
    std::ranges::fill(z_buffer, -std::numeric_limits<double>::infinity());
}

void Rasterizer::set_options(int MSAA) {
    double change_msaa = static_cast<double>(MSAA) / this->MSAA;
    this->MSAA = MSAA;
    height *= change_msaa;
    width  *= change_msaa;
    z_buffer.resize(width * height);
    framebuffer.resize(width * height);
    clear_buffer();
}

//TODO: 光栅化管线
//TODO: 目前透视投影前z=0的点经过透视后w=0, 导致无法投影到屏幕上，应该实现裁剪来处理

void Rasterizer::rasterize_scene(Scene &scene) {
    view = scene.camera.get_view_matrix();
    projection = scene.camera.get_projection_matrix();
    viewport = scene.camera.get_viewport_matrix();
    lights = scene.lights;
    for (const Model& obj_model: scene.models) {
        model = obj_model.get_transform_matrix();
        mvpv = viewport * projection * view * model;
        mv = view * model;
        mvit = (view * model).invert().transpose();
        pre_z(obj_model);
        rasterize_model(obj_model);
    }
}

void Rasterizer::rasterize_model(const Model& obj_model) {
    texture = obj_model.material.texture;
    normal_map = obj_model.material.normal_map;
    bool diffuse_mapping = obj_model.material.diffuse_mapping;
    NormalType normal_type = obj_model.material.normal_type;
    ShadeFrequency shade_frequency = obj_model.material.shade_frequency;

    size_t tri_count = obj_model.triangles.size();
#pragma omp parallel for default(none) shared(obj_model, tri_count, diffuse_mapping, normal_type, shade_frequency, mvit, mvpv, mv, width, height, framebuffer, z_buffer, lights, texture, normal_map, fragment_shader)
    for (size_t idx = 0; idx < tri_count; ++idx) {
        shader_payload payload;
        payload.light_info = lights;
        payload.properties = obj_model.material.properties;


        triangle now_triangle = obj_model.triangles[idx];
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

                    payload.position = now_triangle.get_interpolated_world_position();
                    payload.tex_coords = now_triangle.get_interpolated_tex_coords();

                    if (diffuse_mapping) {
                        payload.color = texture.get_bilinear(payload.tex_coords);
                    } else {
                        payload.color = {0.5, 0.5, 0.5};
                    }

                    {if (normal_type == GLOBAL) {
                        payload.normal = normal_map.get_bilinear(payload.tex_coords) * 2 - vec3{1, 1, 1};
                    } else if (shade_frequency == PER_FRAGMENT) {
                        payload.normal = now_triangle.get_interpolated_normal();
                    } else {
                        payload.normal = normalize((now_triangle.world_vertices[1] - now_triangle.world_vertices[0]) ^
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
                            vec3 n = payload.normal;
                            mat<3, 3> TBN{{{t.x, b.x, n.x}, {t.y, b.y, n.y}, {t.z, b.z, n.z}}};
                            payload.normal = normalize(
                                TBN * (normal_map.get_bilinear(payload.tex_coords) * 2 - vec3{1, 1, 1}));
                        }
                    }}

                    framebuffer[get_index(x, y)] = fragment_shader->shade(payload);
                }
            }
        }
    }
}

void Rasterizer::pre_z(const Model& obj_model) {
#pragma omp parallel for default(none) shared(obj_model, mvpv, mv, width, height, z_buffer, tile_locks)
    for (auto now_triangle : obj_model.triangles) {
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
                if (z > z_buffer[get_index(x, y)] - 1e-4) {
                    z_buffer[get_index(x, y)] = z;
                }
            }
        }
    }
}

void Rasterizer::draw_on_TGA(TGAImage& framebuffer_) {
    int h = height / MSAA;
    int w = width  / MSAA;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            vec3 color_sum = {};
            for (int dy = 0; dy < MSAA; dy++) {
                for (int dx = 0; dx < MSAA; dx++) {
                    color_sum += framebuffer[get_index(x * MSAA + dx,
                                                     y * MSAA + dy)];
                }
            }
            vec3 color = color_sum / MSAA / MSAA;
            framebuffer_.set(x, y, TGAColor(color));
        }
    }
}

