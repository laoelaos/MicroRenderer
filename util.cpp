//
// Created by laoe on 25-9-7.
//

#include "util.h"

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

// clamp the value to [0, 255] and convert to unsigned char
TGAColor vec3_to_color(const vec3& color) {
    return {static_cast<unsigned char>(std::max(0., std::min(255., color.x * 255.))),
            static_cast<unsigned char>(std::max(0., std::min(255., color.y * 255.))),
            static_cast<unsigned char>(std::max(0., std::min(255., color.z * 255.))),
            255};
}

// convert TGAColor to vec3 [b, g, r]
vec3 color_to_vec3(const TGAColor& color) {
    return vec3{static_cast<double>(color.bgra[0]), static_cast<double>(color.bgra[1]), static_cast<double>(color.bgra[2])} / 255.0;
}

// convert normal map color to vec3 [r, g, b]
vec3 nor_color_to_vec3(const TGAColor& color) {
    return vec3{static_cast<double>(color.bgra[2]), static_cast<double>(color.bgra[1]), static_cast<double>(color.bgra[0])} / (255.0 / 2) - vec3{1, 1, 1};
}
