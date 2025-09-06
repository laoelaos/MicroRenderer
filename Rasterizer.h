//
// Created by laoe on 25-9-6.
//

#ifndef RASTERIZER_H
#define RASTERIZER_H
#include <vector>

#include "geometry.h"
#include "tgaimage.h"

class Rasterizer {
public:
    Rasterizer(int w, int h);
    void clear();

    void load_vertices(const std::vector<vec3>& vertices);
    void load_indices(const std::vector<int>& indices);
    void set_model_matrix(const mat4& m) { model = m; }
    void set_view_matrix(const mat4& m) { view = m; }
    void set_projection_matrix(const mat4& m) { projection = m;}

    void rasterize();
    void drawonTGA(TGAImage& framebuffer);
private:
    [[nodiscard]] int get_index(int x, int y) const { return x + y * width; }
    void rasterize_triangle(vec4 v4s[], vec3 color);
private:
    mat4 model, view, projection, viewport;
    int width, height;
    std::vector<vec3> vertices;
    std::vector<int> indices; // each 3 int is a triangle

    std::vector<double> z_buffer;
    std::vector<vec3> framebuffer;
};

#endif //RASTERIZER_H
