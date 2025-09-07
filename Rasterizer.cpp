//
// Created by laoe on 25-9-6.
//

#include <algorithm>

#include "Rasterizer.h"
#include "util.h"

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
    for (triangle triangle_ : triangles) { // iterate through all triangles
        triangle new_triangle = triangle_;
        mat4 invert_transpose = (view * model).invert().transpose();
        vec3 world_pos[3] = {};

        for (int i = 0; i < 3; i++) {
            new_triangle.vertices[i] = (mvpv * (triangle_.vertices[i].to_vec4(1.0))).to_vec3_point();
            new_triangle.normals[i] = normalize((invert_transpose * triangle_.normals[i].to_vec4(0.0)).to_vec3_vec());

            world_pos[i] = (mv * triangle_.vertices[i].to_vec4(1.0)).to_vec3_point();
        }

        rasterize_triangle(new_triangle, world_pos);
    }
}

void Rasterizer::rasterize_triangle(triangle triangle_, vec3 world_pos[3]) {

    auto [x_min, x_max] = std::minmax({triangle_.vertices[0].x, triangle_.vertices[1].x, triangle_.vertices[2].x});
    auto [y_min, y_max] = std::minmax({triangle_.vertices[0].y, triangle_.vertices[1].y, triangle_.vertices[2].y});
    x_min = std::max<int>(0, std::floor(x_min));
    x_max = std::min<int>(width - 1, std::ceil(x_max));
    y_min = std::max<int>(0, std::floor(y_min));
    y_max = std::min<int>(height - 1, std::ceil(y_max));

    shader_payload payload;
    payload.light_info = {{{20, 20, 20}, {2000, 2000, 2000}}};
    payload.texture = texture;
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            auto [alpha, beta, gamma] = compute_barycentric_2D(x + .5, y + .5, triangle_.vertices.data());
            if (alpha<0 || beta<0 || gamma<0)
                continue;

            double w_reciprocal = 1.0 / (alpha + beta + gamma);
            double z = alpha * triangle_.vertices[0].z + beta * triangle_.vertices[1].z + gamma * triangle_.vertices[2].z;
            z *= w_reciprocal;

            double z0 = 1.0/world_pos[0].z;
            double z1 = 1.0/world_pos[1].z;
            double z2 = 1.0/world_pos[2].z;

            double c_alpha = alpha * z0 / (alpha * z0 + beta * z1 + gamma * z2);
            double c_beta = beta * z1 / (alpha * z0 + beta * z1 + gamma * z2);
            double c_gamma = gamma * z2 / (alpha * z0 + beta * z1 + gamma * z2);


            if (z > z_buffer[get_index(x, y)]) {
                payload.position = c_alpha * world_pos[0] + c_beta * world_pos[1] + c_gamma * world_pos[2];

                if (smooth_shading) {
                    payload.normal = normalize(c_alpha * triangle_.normals[0] + c_beta * triangle_.normals[1] + c_gamma * triangle_.normals[2]);
                } else {
                    payload.normal = normalize((world_pos[1] - world_pos[0]) ^ (world_pos[2] - world_pos[1]));
                }

                payload.tex_coords = c_alpha * triangle_.tex_coords[0] + c_beta * triangle_.tex_coords[1] + c_gamma * triangle_.tex_coords[2];

                framebuffer[get_index(x, y)] = fragment_shader->shade(payload);
                z_buffer[get_index(x, y)] = z;
            }
        }
    }
}

void Rasterizer::drawonTGA(TGAImage& framebuffer_) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            framebuffer_.set(x, y, framebuffer[get_index(x, y)].to_color());
        }
    }
}
