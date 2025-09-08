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
#include "Shader.h"

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
    vec3 x = normalize(z ^ up);
    vec3 y = normalize(x ^ z);
    mat4 rotate {{{x.x, x.y, x.z, 0},
                        {y.x, y.y, y.z, 0},
                        {-z.x, -z.y, -z.z, 0},
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

mat4 perspective_projection(const double fov, const double aspect, double near, double far) {
    double top = near * std::tan(fov * M_PI / 360.0);
    double bottom = -top;
    double right = top * aspect;
    double left = -right;
    near = -near;
    far  = -far;
    mat4 orth = orthographic_projection(near, far, right, left, top, bottom);
    mat4 pers {{{near, 0, 0, 0},
                        {0, near, 0, 0},
                        {0, 0, near + far, -near*far},
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
    constexpr double fov = 55.0;
    constexpr double aspect = 1.;
    constexpr double near = 2;
    constexpr double far  = 3;

    constexpr int width  = 2000;
    constexpr int height = 2000;

    const vec3 eye    = {0, 0, 1};
    const vec3 center = {0, 0, 2};
    const vec3 up     = {0, 1, 0};

    //projection = orthographic_projection(2, 3, aspect, -aspect, 1, -1);
    //perspective_projection(fov, aspect, near, far)

    Rasterizer rasterizer(width, height);

    rasterizer.set_model_matrix(model_matrix());
    rasterizer.set_view_matrix(view_matrix(eye, center, up));
    rasterizer.set_projection_matrix(perspective_projection(fov, aspect, near, far));

    rasterizer.load_fragment_shader(std::make_shared<PhongShader_Texture>());
    rasterizer.set_options(true, true);

    for (int i = 1; i < argc; i++)
    {
        Model model(argv[i]);
        rasterizer.load_triangles(model.triangles);
        TGAImage texture, normal_map;
        std::string base = argv[i];
        base.resize(base.size()-4);
        texture.read_tga_file(base + std::string("_diffuse.tga"));
        rasterizer.set_texture(texture);
        normal_map.read_tga_file(base + std::string("_nm.tga"));
        rasterizer.set_normal_map(normal_map);

        rasterizer.rasterize();
        rasterizer.clear_triangles();
    }



    TGAImage framebuffer(width, height, TGAImage::RGB);
    rasterizer.drawonTGA(framebuffer);
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}

