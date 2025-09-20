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

std::tuple<int, int, int, int> find_bounding_box_int(const std::array<vec3, 3> v3s, int width, int height) {
    auto [x_min, x_max] = std::minmax({v3s[0].x, v3s[1].x, v3s[2].x});
    auto [y_min, y_max] = std::minmax({v3s[0].y, v3s[1].y, v3s[2].y});
    x_min = std::max(0, static_cast<int>(std::floor(x_min)));
    x_max = std::min(width - 1, static_cast<int>(std::ceil(x_max)));
    y_min = std::max(0, static_cast<int>(std::floor(y_min)));
    y_max = std::min(height - 1, static_cast<int>(std::ceil(y_max)));
    return {x_min, x_max, y_min, y_max};
}
