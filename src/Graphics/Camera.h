//
// Created by laoe on 2025/10/12.
//

#ifndef MICRORENDERER_CAMERA_H
#define MICRORENDERER_CAMERA_H

#include "Geometry.h"

class Camera {
public:
    vec3 eye = {0, 0, 0};
    vec3 center = {0, 0, 2};
    vec3 up = {0, 1, 0};

    int width = 600;
    int height = 600;

    double fov = 55.0;
    double aspect = 1.0;
    double near_ = 2.0;
    double far_ = 3.0;

    double yaw = 0.0;
    double pitch = 0.0;
    double roll = 0.0;

    Camera() = default;
    Camera(const vec3& eye_, const vec3& center_, const vec3& up_, int w = 600, int h = 600,
           double fov_ = 55.0, double near_ = 2.0, double far_ = 3.0);

    void update();
    [[nodiscard]] mat4 get_view_matrix() const;
    [[nodiscard]] mat4 get_projection_matrix() const;
    [[nodiscard]] mat4 get_viewport_matrix() const;
    //TODO: 欧拉角旋转好像有点问题
    /**
     *  @brief 设置相机朝向，基于相机位置旋转，顺时针旋转
     *  @param yaw 水平旋转角度，单位度
     *  @param pitch 垂直旋转角度，单位度
     *  @param roll 侧滚旋转角度，单位度
     */
    void set_toward_from_center(double yaw, double pitch, double roll);

    /**
     *  @brief 设置相机朝向，基于观察点旋转，顺时针旋转
     *  @param yaw 水平旋转角度，单位度
     *  @param pitch 垂直旋转角度，单位度
     *  @param roll 侧滚旋转角度，单位度
     */
    void set_toward_from_eye(double yaw, double pitch, double roll);

    /**
     *  @brief 设置相机朝向增量，基于相机位置旋转，顺时针旋转
     *  @param yaw 水平旋转角度，单位度
     *  @param pitch 垂直旋转角度，单位度
     *  @param roll 侧滚旋转角度，单位度
     */
    void rotate_around_center(double yaw, double pitch, double roll);

    /**
     *  @brief 设置相机朝向增量，基于观察点旋转，顺时针旋转
     *  @param yaw 水平旋转角度，单位度
     *  @param pitch 垂直旋转角度，单位度
     *  @param roll 侧滚旋转角度，单位度
     */
    void rotate_around_eye(double yaw, double pitch, double roll);
};

#endif //MICRORENDERER_CAMERA_H