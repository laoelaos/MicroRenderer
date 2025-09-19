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
 * @param color 代表法线的TGAColor [b, g, r, a] 范围：[0, 255]
 * @return normalized vec3 [x(r), y(g), z(b)] 范围：[-1, 1]
 */
vec3 nor_color_to_vec3(const TGAColor& color);

/**
 * @brief 从法线贴图中获取法线向量
 * @param image 法线贴图
 * @param uv 纹理坐标
 * @return normalized vec3 [x(r), y(g), z(b)] 范围：[-1, 1]
 */
vec3 get_nor_vec3_from_tga(const TGAImage& image, const vec2 &uv);

/**
 * @brief 双线性插值从法线贴图中获取法线向量
 * @param image 法线贴图
 * @param uv 纹理坐标
 * @return normalized vec3 [x(r), y(g), z(b)] 范围：[-1, 1]
 */
vec3 get_nor_vec3_from_tga_bilinear(const TGAImage& image, const vec2 &uv);

/**
 * @brief 直接从纹理贴图中获取颜色向量
 * @param image 纹理贴图
 * @param uv 纹理坐标
 * @return vec3 [r, g, b] 范围：[0, 1]
 */
vec3 get_color_vec3_from_tga(const TGAImage& image, const vec2 &uv);

/**
 * @brief 双线性插值从纹理贴图中获取颜色向量
 * @param image 纹理贴图
 * @param uv 纹理坐标
 * @return vec3 [r, g, b] 范围：[0, 1]
 */
vec3 get_color_vec3_from_tga_bilinear(const TGAImage& image, const vec2 &uv);

/**
 * @brief 计算变换矩阵
 * @param translation 平移向量
 * @param rotation 旋转向量（角度制，绕X、Y、Z轴旋转）
 * @param scale 缩放向量
 * @return 变换矩阵
 */
mat4 calculate_transform_matrix(const vec3& translation, const vec3& rotation, const vec3& scale);

/**
 * @return 单位矩阵
 */
mat4 model_matrix();

/**
 * @brief 计算视图矩阵
 * @param eye 观察点位置
 * @param center 相机所在位置
 * @param up 相机上方向
 * @return 视图矩阵
 */
mat4 view_matrix(const vec3 &eye, const vec3 &center, const vec3 &up);

/**
 * @brief 计算正交投影矩阵
 * @param near_ 近平面（正数）
 * @param far_ 远平面（正数）
 * @param right 右边界
 * @param left 左边界
 * @param top 上边界
 * @param bottom 下边界
 * @return 正交投影矩阵
 */
mat4 orthographic_projection(double near_, double far_, double right, double left, double top, double bottom);

/**
 * @brief 计算透视投影矩阵
 * @param fov 视场角（角度制）
 * @param aspect 宽高比
 * @param near_ 近平面（正数）
 * @param far_ 远平面（正数）
 * @return 透视投影矩阵
 */
mat4 perspective_projection(double fov, double aspect, double near_, double far_);

/**
 * @brief 计算视口变换矩阵
 * @param w 视口宽度
 * @param h 视口高度
 * @return 视口变换矩阵
 */
mat4 viewport_matrix(int w, int h);

#endif //UTIL_H
