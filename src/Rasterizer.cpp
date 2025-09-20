//
// Created by laoe on 25-9-6.
//

#include <algorithm>

#include "Rasterizer.h"

#include <iostream>
#include <thread>

#include "Model.h"
#include "Util.h"

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
        const triangle now_triangle = obj_model.triangles[idx];

        shader_payload payload;
        payload.light_info = lights;
        payload.properties = obj_model.material.properties;

        std::array<vec3, 3> normals = {};
        std::array<vec3, 3> vertices_screen_pos = {};
        std::array<vec3, 3> vertices_world_pos = {};
        std::array<vec2, 3> tex_coords = now_triangle.tex_coords;

        for (int i = 0; i < 3; i++) {
            normals[i] = normalize((mvit * now_triangle.normals[i].to_vec4(0.0)).to_vec3_vec());
            vertices_screen_pos[i] = (mvpv * now_triangle.vertices[i].to_vec4(1.0)).to_vec3_point();
            vertices_world_pos[i] = (mv * now_triangle.vertices[i].to_vec4(1.0)).to_vec3_point();
        }

        if (((vertices_screen_pos[1] - vertices_screen_pos[0]) ^ (vertices_screen_pos[2] - vertices_screen_pos[1])).z < 0)
            continue;

        auto [x_min, x_max, y_min, y_max] = find_bounding_box_int(vertices_screen_pos, width, height);

        for (int x = x_min; x <= x_max; x++) {
            for (int y = y_min; y <= y_max; y++) {
                auto [alpha, beta, gamma] = compute_barycentric_2D(x + .5, y + .5, vertices_screen_pos);
                if (alpha<0 || beta<0 || gamma<0)
                     continue;
                double z = alpha * vertices_screen_pos[0].z + beta * vertices_screen_pos[1].z + gamma * vertices_screen_pos[2].z;
                double z0 = 1.0/vertices_world_pos[0].z;
                double z1 = 1.0/vertices_world_pos[1].z;
                double z2 = 1.0/vertices_world_pos[2].z;
                double c_alpha = alpha * z0 / (alpha * z0 + beta * z1 + gamma * z2);
                double c_beta = beta * z1 / (alpha * z0 + beta * z1 + gamma * z2);
                double c_gamma = gamma * z2 / (alpha * z0 + beta * z1 + gamma * z2);

                std::lock_guard guard(tile_locks[get_tile_lock(x, y)]);
                if (z >= z_buffer[get_index(x, y)]) {
                    z_buffer[get_index(x, y)] = z;

                    payload.position = c_alpha * vertices_world_pos[0] + c_beta * vertices_world_pos[1] + c_gamma * vertices_world_pos[2];
                    payload.tex_coords = c_alpha * tex_coords[0] + c_beta * tex_coords[1] + c_gamma * tex_coords[2];

                    if (diffuse_mapping) {
                        payload.color = get_color_vec3_from_tga_bilinear(texture, payload.tex_coords);
                    } else {
                        payload.color = c_alpha * vec3{1, 1, 1} + c_beta * vec3{1, 1, 1} + c_gamma * vec3{1, 1, 1};
                    }

                    if (normal_type == GLOBAL) {
                        payload.normal = get_nor_vec3_from_tga_bilinear(normal_map, payload.tex_coords);
                    } else if (shade_frequency == PER_FRAGMENT) {
                        payload.normal = normalize(c_alpha * normals[0] + c_beta * normals[1] + c_gamma * normals[2]);
                    } else {
                        payload.normal = normalize((vertices_world_pos[1] - vertices_world_pos[0]) ^ (vertices_world_pos[2] - vertices_world_pos[1]));
                    }

                    if (normal_type == TANGENT) {
                        vec3 e1 = vertices_world_pos[1] - vertices_world_pos[0];
                        vec3 e2 = vertices_world_pos[2] - vertices_world_pos[0];
                        vec2 delta_uv1 = tex_coords[1] - tex_coords[0];
                        vec2 delta_uv2 = tex_coords[2] - tex_coords[0];
                        mat<2,2> uv_matrix {{{delta_uv1.x, delta_uv2.x}, {delta_uv1.y, delta_uv2.y}}};
                        mat<3,2> edge_matrix {{{e1.x, e2.x}, {e1.y, e2.y}, {e1.z, e2.z}}};
                        if (std::abs(determinant(uv_matrix)) > 1e-8) {
                            mat<3,2> tnb = edge_matrix * uv_matrix.invert();
                            vec3 t = normalize(tnb.get_col(0));
                            vec3 b = normalize(tnb.get_col(1));
                            vec3 n = payload.normal;
                            mat<3,3> TBN {{{t.x, b.x, n.x}, {t.y, b.y, n.y}, {t.z, b.z, n.z}}};
                            payload.normal = normalize(TBN * get_nor_vec3_from_tga_bilinear(normal_map, payload.tex_coords));
                        }
                    }

                    framebuffer[get_index(x, y)] = fragment_shader->shade(payload);
                }

            }
        }
    }
}

void Rasterizer::pre_z(Model& obj_model) {
    for (size_t idx = 0; idx < obj_model.triangles.size(); ++idx) {
        std::array<vec3, 3> vertices_screen_pos = {};
        for (int i = 0; i < 3; i++) {
            vertices_screen_pos[i] = (mvpv * obj_model.triangles[idx].vertices[i].to_vec4(1.0)).to_vec3_point();
        }
        auto [x_min, x_max, y_min, y_max] = find_bounding_box_int(vertices_screen_pos, width, height);

        for (int x = x_min; x <= x_max; x++) {
            for (int y = y_min; y <= y_max; y++) {
                auto [alpha, beta, gamma] = compute_barycentric_2D(x + .5, y + .5, vertices_screen_pos);
                if (alpha<0 || beta<0 || gamma<0)
                    continue;
                double z = alpha * vertices_screen_pos[0].z + beta * vertices_screen_pos[1].z + gamma * vertices_screen_pos[2].z;

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
            framebuffer_.set(x, y, vec3_to_color(color));
        }
    }
}

