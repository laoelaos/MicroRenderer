//
// Created by 20133 on 25-9-7.
//

#ifndef UTIL_H
#define UTIL_H

#include <tuple>
#include "Geometry.h"

/**
 * @brief 计算一点在三角形中的重心坐标
 * @param x 计算点的x坐标
 * @param y 计算点的y坐标
 * @param v3s 三角形顶点坐标
 * @return 3个 double, alpha, beta, gamma， 分别为第0, 1, 2个顶点的系数\n
 * 如果计算点过远, 返回{-1, -1, -1}
 */
std::tuple<double, double, double> compute_barycentric_2D(double x, double y, std::array<vec3, 3> v3s);

/**
 * @brief 计算三角形的包围盒, 并将其限制在屏幕范围内
 * @param v3s 三角形顶点坐标
 * @param width 屏幕宽度
 * @param height 屏幕高度
 * @return x_min, x_max, y_min, y_max
 */
std::tuple<int, int, int, int> find_bounding_box_int(std::array<vec3, 3> v3s, int width, int height);

#endif //UTIL_H
