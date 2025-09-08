//
// Created by 20133 on 25-9-7.
//

#ifndef UTIL_H
#define UTIL_H

#include <tuple>
#include "geometry.h"

/**
 * @brief 计算一点在三角形中的重心坐标
 * @param x 计算点的x坐标
 * @param y 计算点的y坐标
 * @param v3s 三角形顶点坐标
 * @return 3个 double, alpha, beta, gamma， 分别为第0, 1, 2个顶点的系数\n
 * 如果计算点过远, 返回{-1, -1, -1}
 */
std::tuple<double, double, double> compute_barycentric_2D(double x, double y, const vec3 v3s[3]);

/**
 * @param color vec3 [r, g, b] 范围：[0, 1]
 * @return TGAColor [b, g, r, a] 范围：[0, 255]
 */
TGAColor vec3_to_color(const vec3& color);

/**
 * @param color TGAColor [b, g, r, a] 范围：[0, 255]
 * @return vec3 [r, g, b] 范围：[0, 1]
 */
vec3 color_to_vec3(const TGAColor& color);

/**
 * @param color TGAColor [b, g, r, a] 范围：[0, 255]
 * @return vec3 [x(r), y(g), z(b)] 范围：[-1, 1]
 */
vec3 nor_color_to_vec3(const TGAColor& color);

#endif //UTIL_H
