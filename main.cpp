#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "Rasterizer.h"

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

mat4 model_matrix() {
    return identity_matrix<4>();
}

mat4 view_matrix(const vec3 &eye, const vec3 &center, const vec3 &up) {
    vec3 z = normalize(eye - center);
    vec3 x = normalize(up ^ z);
    vec3 y = normalize(z ^ x);
    mat4 rotate {{{x.x, x.y, x.z, 0},
                        {y.x, y.y, y.z, 0},
                        {z.x, z.y, z.z, 0},
                        {0,   0,   0,   1}}};
    mat4 translate {{{1, 0, 0, -center.x},
                        {0, 1, 0, -center.y},
                        {0, 0, 1, -center.z},
                        {0, 0, 0, 1}}};
    return rotate * translate;
}

mat4 orthographic_projection(const double near, const double far, const double right, const double left, const double top, const double bottom) {
    mat4 translate {{{1, 0, 0, -(left + right) / 2},
                        {0, 1, 0, -(top + bottom) / 2},
                        {0, 0, 1, -(near + far) / 2},
                        {0, 0, 0, 1}}};
    mat4 scale {{{2 / (right - left), 0, 0, 0},
                        {0, 2 / (top - bottom), 0, 0},
                        {0, 0, 2 / (near - far), 0},
                        {0, 0, 0, 1}}};
    return scale * translate;
}

mat4 perspective_projection(const double fov, const double aspect, const double near, const double far) {
    double top = near * std::tan(fov / 2 * M_PI / 360.0);
    double bottom = -top;
    double right = top * aspect;
    double left = -right;
    mat4 orth = orthographic_projection(near, far, right, left, top, bottom);
    mat4 pers {{{far, 0, 0, 0},
                        {0, far, 0, 0},
                        {0, 0, near +far, -near*far},
                        {0, 0, 1, 0}}};
    return orth * pers;
}

mat4 viewport_matrix(int w, int h) {
    return {{{w/2., 0, 0, w/2.},
                        {0, h/2., 0, h/2.},
                        {0, 0, 1, 0},
                        {0, 0, 0, 1}}};
}

int main(int argc, char** argv) {
    constexpr double fov = 150.0;
    constexpr double aspect = 16.0 / 9.0;
    constexpr double near = 2;
    constexpr double far  = 3;

    constexpr int width  = 1600;
    constexpr int height = 900;

    constexpr vec3 eye    = {0, 0, 1};
    constexpr vec3 center = {0, 0, 2};
    constexpr vec3 up     = {0, 1, 0};

    //projection = orthographic_projection(2, 3, aspect, -aspect, 1, -1);

    Rasterizer rasterizer(width, height);

    rasterizer.set_model_matrix(model_matrix());
    rasterizer.set_view_matrix(view_matrix(eye, center, up));
    rasterizer.set_projection_matrix(perspective_projection(fov, aspect, near, far));

    Model model(argv[1]);
    rasterizer.load_vertices(model.vertices);
    rasterizer.load_indices(model.faces);

    rasterizer.rasterize();

    TGAImage framebuffer(width, height, TGAImage::RGB);
    rasterizer.drawonTGA(framebuffer);
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}

