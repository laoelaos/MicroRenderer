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

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};


mat4 model, view, projection, viewport;

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

std::tuple<double, double, double> compute_barycentric_2D(double x, double y, const vec3 v3s[3]) {
    double alpha_denominator = - (v3s[0].x - v3s[1].x) * (v3s[2].y - v3s[1].y) + (v3s[0].y - v3s[1].y) * (v3s[2].x - v3s[1].x);
    double beta_denominator = - (v3s[1].x - v3s[2].x) * (v3s[0].y - v3s[2].y) + (v3s[1].y - v3s[2].y) * (v3s[0].x - v3s[2].x);
    // check if the denominators are too small even zero
    if (std::abs(alpha_denominator) < 1e-8 || std::abs(beta_denominator) < 1e-8)
        return {-1, -1, -1};

    double alpha = (- (x - v3s[1].x) * (v3s[2].y - v3s[1].y) + (y - v3s[1].y) * (v3s[2].x - v3s[1].x)) / alpha_denominator;
    double beta = (- (x - v3s[2].x) * (v3s[0].y - v3s[2].y) + (y - v3s[2].y) * (v3s[0].x - v3s[2].x)) / beta_denominator;
    double gamma = 1. - alpha - beta;

    return {alpha, beta, gamma};
}

void rasterize(const vec4 v4s[3], TGAImage& framebuffer, std::vector<double>& z_buffer, TGAColor color) {

    vec3 v3s[3];
    for (int i = 0; i < 3; i++) {
        v3s[i] = (viewport * projection * view * model * v4s[i]).to_vec3();
    }

    auto [x_min, x_max] = std::minmax({v3s[0].x, v3s[1].x, v3s[2].x});
    auto [y_min, y_max] = std::minmax({v3s[0].y, v3s[1].y, v3s[2].y});
    x_min = std::max<int>(0, std::floor(x_min));
    x_max = std::min<int>(framebuffer.width() - 1, std::ceil(x_max));
    y_min = std::max<int>(0, std::floor(y_min));
    y_max = std::min<int>(framebuffer.height() - 1, std::ceil(y_max));

    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            auto [alpha, beta, gamma] = compute_barycentric_2D(x, y, v3s);
            if (alpha<0 || beta<0 || gamma<0) 
                continue;

            double z = alpha * v3s[0].z + beta * v3s[1].z + gamma * v3s[2].z;
            if (z > z_buffer[x+y*framebuffer.width()]) {
                framebuffer.set(x, y, color);
                z_buffer[x+y*framebuffer.width()] = z;
            }
        }
    }
}

int main(int argc, char** argv) {
    constexpr double fov = 150.0;
    constexpr double aspect = 1.0;
    constexpr double near = 2;
    constexpr double far  = 3;

    constexpr int width  = 1000;
    constexpr int height = 1000;

    constexpr vec3 eye    = {0, 0, 1};
    constexpr vec3 center = {0, 0, 2};
    constexpr vec3 up     = {0, 1, 0};

    model = model_matrix();
    view = view_matrix(eye, center, up);
    projection = perspective_projection(fov, aspect, near, far);
    viewport = viewport_matrix(width, height);

    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<double> z_buffer(width*height, -std::numeric_limits<double>::max());

    Model model(argv[1]);

    for (int i=0; i<model.getNumberFace(); i++) { // iterate through all triangles
        vec3 v0t1 = model.getVertex(i, 1) - model.getVertex(i, 0);
        vec3 v1t2 = model.getVertex(i, 2) - model.getVertex(i, 1);
        vec3 n = normalize(v0t1 ^ v1t2);

        TGAColor rnd{static_cast<unsigned char>(n.x * 255), static_cast<unsigned char>(n.y * 255), static_cast<unsigned char>(n.z * 255), 255};
        vec4 v4s[3] = {model.getVertex(i, 0).to_vec4(1.), model.getVertex(i, 1).to_vec4(1.), model.getVertex(i, 2).to_vec4(1.)};
        rasterize(v4s, framebuffer, z_buffer, rnd);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}

