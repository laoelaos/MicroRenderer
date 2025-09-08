//
// Created by laoe on 25-9-6.
//

#include <algorithm>

#include "Rasterizer.h"
#include "Material.h"
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
    triangles = {};
    lights = {};
    texture = {};
    normal_map = {};
    fragment_shader = nullptr;
    std::ranges::fill(framebuffer, vec3());
    std::ranges::fill(z_buffer, -std::numeric_limits<double>::infinity());
}

void Rasterizer::load_triangles(const std::vector<triangle>& triangles_) {
    triangles.insert(triangles.end(), triangles_.begin(), triangles_.end());
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
    for (triangle now_triangle: obj_model.triangles) {
        Material material = obj_model.material;
        bool smooth_shading = material.smooth_shading;
        bool normal_mapping = material.normal_mapping;

        std::array<vec3, 3> normals = {};
        std::array<vec3, 3> vertices_screen_pos = {};
        std::array<vec3, 3> vertices_world_pos = {};
        std::array<vec2, 3> tex_coords = now_triangle.tex_coords;

        for (int i = 0; i < 3; i++) {
            normals[i] = normalize((mvit * now_triangle.normals[i].to_vec4(0.0)).to_vec3_vec());
            vertices_screen_pos[i] = (mvpv * (now_triangle.vertices[i].to_vec4(1.0))).to_vec3_point();
            vertices_world_pos[i] = (mv * now_triangle.vertices[i].to_vec4(1.0)).to_vec3_point();
        }

        auto [x_min, x_max, y_min, y_max] = find_box_int(vertices_screen_pos, width, height);

        shader_payload payload;
        payload.light_info = lights;
        for (int x = x_min; x <= x_max; x++) {
            for (int y = y_min; y <= y_max; y++) {
                auto [alpha, beta, gamma] = compute_barycentric_2D(x + .5, y + .5, vertices_screen_pos);
                if (alpha<0 || beta<0 || gamma<0)
                    continue;

                double w_reciprocal = 1.0 / (alpha + beta + gamma);
                double z = alpha * vertices_screen_pos[0].z + beta * vertices_screen_pos[1].z + gamma * vertices_screen_pos[2].z;
                z *= w_reciprocal;
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
                        payload.normal = normalize(nor_color_to_vec3(get_color_from_tga_uv(material.normal_map, payload.tex_coords)));
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
