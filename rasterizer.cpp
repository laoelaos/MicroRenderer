//
// Created by laoe on 25-9-6.
//

#include "rasterizer.h"
#include "tgaimage.h"

rasterizer::rasterizer(int w, int h) : width(w), height(h) {
    z_buffer.resize(w * h);
    framebuffer.resize(w * h);
    clear();
}

void rasterizer::clear() {
    model = identity_matrix<4>();
    view = identity_matrix<4>();
    projection = identity_matrix<4>();
    viewport = {{{width/2., 0,   0, width/2.},
                        {0,   height/2., 0, height/2.},
                        {0,   0,   1,   0},
                        {0,   0,   0,   1}}};
    vertices = {};
    indices = {};
    std::fill(framebuffer.begin(), framebuffer.end(), vec4());
    std::fill(z_buffer.begin(), z_buffer.end(), -std::numeric_limits<double>::infinity());
}

void rasterizer::load_vertices(const std::vector<vec3>& vertices_) {
    vertices.insert(vertices.end(), vertices_.begin(), vertices_.end());
}

void rasterizer::load_indices(const std::vector<int>& indices_) {
    indices.insert(indices.end(), indices_.begin(), indices_.end());
}

void rasterizer::rasterize() {

}

void rasterizer::drawonTGA(TGAImage& framebuffer_) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            framebuffer_.set(x, y, framebuffer[get_index(x, y)].to_color());
        }
    }
}

void rasterizer::rasterize_triangle() {

}
