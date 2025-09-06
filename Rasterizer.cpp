//
// Created by laoe on 25-9-6.
//

#include <algorithm>

#include "Rasterizer.h"
#include "util.h"

Rasterizer::Rasterizer(int w, int h) : width(w), height(h) {
    z_buffer.resize(w * h);
    framebuffer.resize(w * h);
    clear();
}

void Rasterizer::clear() {
    model = identity_matrix<4>();
    view = identity_matrix<4>();
    projection = identity_matrix<4>();
    viewport = {{{width/2., 0,   0, width/2.},
                        {0,   height/2., 0, height/2.},
                        {0,   0,   1,   0},
                        {0,   0,   0,   1}}};
    vertices = {};
    indices = {};
    std::fill(framebuffer.begin(), framebuffer.end(), vec3());
    std::fill(z_buffer.begin(), z_buffer.end(), -std::numeric_limits<double>::infinity());
}

void Rasterizer::load_vertices(const std::vector<vec3>& vertices_) {
    vertices.insert(vertices.end(), vertices_.begin(), vertices_.end());
}

void Rasterizer::load_indices(const std::vector<int>& indices_) {
    indices.insert(indices.end(), indices_.begin(), indices_.end());
}

void Rasterizer::rasterize() {
    int nface = static_cast<int>(indices.size()) / 3;
    for (int i=0; i < nface; i++) { // iterate through all triangles
        vec3 v0t1 = vertices[indices[i*3+1]] - vertices[indices[i*3+0]];
        vec3 v1t2 = vertices[indices[i*3+2]] - vertices[indices[i*3+1]];
        vec3 n = normalize(v0t1 ^ v1t2);

        vec3 normal_color {n.x * 255, n.y * 255, n.z * 255};
        vec4 v4s[3] = {vertices[indices[i*3+0]].to_vec4(1.), vertices[indices[i*3+1]].to_vec4(1.), vertices[indices[i*3+2]].to_vec4(1.)};
        rasterize_triangle(v4s, normal_color);
    }
}

void Rasterizer::drawonTGA(TGAImage& framebuffer_) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            framebuffer_.set(x, y, framebuffer[get_index(x, y)].to_color());
        }
    }
}

void Rasterizer::rasterize_triangle(vec4 v4s[3], vec3 color) {
    vec3 v3s[3];
    for (int i = 0; i < 3; i++) {
        v3s[i] = (viewport * projection * view * model * v4s[i]).to_vec3();
    }

    auto [x_min, x_max] = std::minmax({v3s[0].x, v3s[1].x, v3s[2].x});
    auto [y_min, y_max] = std::minmax({v3s[0].y, v3s[1].y, v3s[2].y});
    x_min = std::max<int>(0, std::floor(x_min));
    x_max = std::min<int>(width - 1, std::ceil(x_max));
    y_min = std::max<int>(0, std::floor(y_min));
    y_max = std::min<int>(height - 1, std::ceil(y_max));

    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            auto [alpha, beta, gamma] = compute_barycentric_2D(x + .5, y + .5, v3s);
            if (alpha<0 || beta<0 || gamma<0)
                continue;

            double z = alpha * v3s[0].z + beta * v3s[1].z + gamma * v3s[2].z;
            if (z > z_buffer[get_index(x, y)]) {
                framebuffer[get_index(x, y)] = color;
                z_buffer[get_index(x, y)] = z;
            }
        }
    }
}
