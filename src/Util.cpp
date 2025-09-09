//
// Created by laoe on 25-9-7.
//

#include "Util.h"

#include <algorithm>
#include <array>

std::tuple<double, double, double> compute_barycentric_2D(double x, double y, std::array<vec3, 3> v3s) {
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

TGAColor vec3_to_color(const vec3& color) {
    return {static_cast<unsigned char>(std::max(0., std::min(255., color.x * 255.))),
            static_cast<unsigned char>(std::max(0., std::min(255., color.y * 255.))),
            static_cast<unsigned char>(std::max(0., std::min(255., color.z * 255.))),
            255};
}

vec3 color_to_vec3(const TGAColor& color) {
    return vec3{static_cast<double>(color.bgra[0]), static_cast<double>(color.bgra[1]), static_cast<double>(color.bgra[2])} / 255.0;
}

vec3 nor_color_to_vec3(const TGAColor& color) {
    return normalize(vec3{static_cast<double>(color.bgra[2]), static_cast<double>(color.bgra[1]), static_cast<double>(color.bgra[0])} / (255.0 / 2) - vec3{1, 1, 1});
}

vec3 get_nor_vec3_from_tga(const TGAImage& image, const vec2 &uv) {
    return nor_color_to_vec3(image.get(static_cast<int>(uv.x * image.width()), static_cast<int>(uv.y * image.height())));
}

vec3 get_nor_vec3_from_tga_bilinear(const TGAImage& image, const vec2 &uv) {
    vec3 c = get_color_vec3_from_tga_bilinear(image, uv);
    return normalize(vec3{c[2], c[1], c[0]} * 2 - vec3{1, 1, 1});
}

vec3 get_color_vec3_from_tga(const TGAImage& image, const vec2 &uv) {
    return color_to_vec3(image.get(static_cast<int>(uv.x * image.width()), static_cast<int>(uv.y * image.height())));
}

vec3 get_color_vec3_from_tga_bilinear(const TGAImage& image, const vec2 &uv) {
    int x = static_cast<int>(uv.x * image.width());
    int y = static_cast<int>(uv.y * image.height());
    int xp = std::min(x + 1, image.width() - 1);
    int yp = std::min(y + 1, image.height() - 1);
    double u = uv.x * image.width() - x;
    double v = uv.y * image.height() - y;
    vec3 c00 = color_to_vec3(image.get(x, y));
    vec3 c10 = color_to_vec3(image.get(xp, y));
    vec3 c01 = color_to_vec3(image.get(x, yp));
    vec3 c11 = color_to_vec3(image.get(xp, yp));
    return (1 - u) * (1 - v) * c00 +
           u * (1 - v) * c10 +
           (1 - u) * v * c01 +
           u * v * c11;
}

std::tuple<int, int, int, int> find_bounding_box_int(const std::array<vec3, 3> v3s, int width, int height) {
    auto [x_min, x_max] = std::minmax({v3s[0].x, v3s[1].x, v3s[2].x});
    auto [y_min, y_max] = std::minmax({v3s[0].y, v3s[1].y, v3s[2].y});
    x_min = std::max(0, static_cast<int>(std::floor(x_min)));
    x_max = std::min(width - 1, static_cast<int>(std::ceil(x_max)));
    y_min = std::max(0, static_cast<int>(std::floor(y_min)));
    y_max = std::min(height - 1, static_cast<int>(std::ceil(y_max)));
    return {x_min, x_max, y_min, y_max};
}
