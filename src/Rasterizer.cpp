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
    clear_all();
}

void Rasterizer::clear_all() {
    model = identity_matrix<4>();
    view = identity_matrix<4>();
    projection = identity_matrix<4>();
    viewport = {{{width/2., 0,   0, width/2.},
                        {0,   height/2., 0, height/2.},
                        {0,   0,   1,   0},
                        {0,   0,   0,   1}}};
    models = {};
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

void Rasterizer::load_lights(const std::vector<light> &lights_) {
    lights.insert(lights.end(), lights_.begin(), lights_.end());
}


void Rasterizer::rasterize() {
    mvpv = viewport * projection * view * model;
    mv = view * model;
    mvit = (view * model).invert().transpose();
    for (const Model& obj_model: models) {
        rasterize_model(obj_model);
    }

    TGAImage output(width, height, TGAImage::RGB);
    drawonTGA(output);
    if (!output.write_tga_file("output.tga")) {
        throw std::runtime_error("Failed to write output.tga");
    }
}

void Rasterizer::rasterize_model(const Model& obj_model) {
    texture = obj_model.material.texture;
    normal_map = obj_model.material.normal_map;
    bool smooth_shading = obj_model.material.smooth_shading;
    bool normal_mapping = obj_model.material.normal_mapping;

    shader_payload payload;
    payload.light_info = lights;
    payload.texture = texture;
    payload.k_ambient = obj_model.material.k_ambient;
    payload.k_diffuse = obj_model.material.k_diffuse;
    payload.k_specular = obj_model.material.k_specular;
    payload.p = obj_model.material.p;
    for (triangle now_triangle: obj_model.triangles) {
        std::array<vec3, 3> normals = {};
        std::array<vec3, 3> vertices_screen_pos = {};
        std::array<vec3, 3> vertices_world_pos = {};
        std::array<vec2, 3> tex_coords = now_triangle.tex_coords;

        for (int i = 0; i < 3; i++) {
            normals[i] = normalize((mvit * now_triangle.normals[i].to_vec4(0.0)).to_vec3_vec());
            vertices_screen_pos[i] = (mvpv * (now_triangle.vertices[i].to_vec4(1.0))).to_vec3_point();
            vertices_world_pos[i] = (mv * now_triangle.vertices[i].to_vec4(1.0)).to_vec3_point();
        }

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

                if (z > z_buffer[get_index(x, y)]) {
                    payload.position = c_alpha * vertices_world_pos[0] + c_beta * vertices_world_pos[1] + c_gamma * vertices_world_pos[2];
                    payload.tex_coords = c_alpha * tex_coords[0] + c_beta * tex_coords[1] + c_gamma * tex_coords[2];

                    if (normal_mapping) {
                        payload.normal = get_vec3_nor_from_tga_uv(normal_map, payload.tex_coords);
                    } else if (smooth_shading) {
                        payload.normal = normalize(c_alpha * normals[0] + c_beta * normals[1] + c_gamma * normals[2]);
                    } else {
                        payload.normal = normalize((vertices_world_pos[1] - vertices_world_pos[0]) ^ (vertices_world_pos[2] - vertices_world_pos[1]));
                    }

                    framebuffer[get_index(x, y)] = fragment_shader->shade(payload);
                    z_buffer[get_index(x, y)] = z;
                }
            }
        }
    }
}

void Rasterizer::drawonTGA(TGAImage& framebuffer_) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            framebuffer_.set(x, y, vec3_to_color(framebuffer[get_index(x, y)]));
        }
    }
}
