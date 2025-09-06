//
// Created by 20133 on 25-9-7.
//

#ifndef UTIL_H
#define UTIL_H

#include <tuple>
#include "geometry.h"

std::tuple<double, double, double> compute_barycentric_2D(double x, double y, const vec3 v3s[3]);

#endif //UTIL_H
