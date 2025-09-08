//
// Created by 20133 on 25-9-7.
//

#ifndef UTIL_H
#define UTIL_H

#include <tuple>
#include "geometry.h"

std::tuple<double, double, double> compute_barycentric_2D(double x, double y, const vec3 v3s[3]);
TGAColor vec3_to_color(const vec3& color);
vec3 color_to_vec3(const TGAColor& color);
vec3 nor_color_to_vec3(const TGAColor& color);

#endif //UTIL_H
