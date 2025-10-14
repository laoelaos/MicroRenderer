//
// Created by laoe on 2025/10/12.
//

#ifndef MICRORENDERER_CAMERA_H
#define MICRORENDERER_CAMERA_H

#include "Geometry.h"

//目前eye与center的关系可能相反
class Camera {
    vec3 m_eye = {0, 0, 0}; // 观察点位置, 目标位置
    vec3 m_center = {0, 0, 3}; // 相机位置
    vec3 m_up = {0, 1, 0};

    int m_width = 600;
    int m_height = 600;

    double m_fov = 55.0;
    double m_aspect = 1.0;
    double m_near = 2.0;
    double m_far = 3.0;

    double m_yaw = -90.0;
    double m_pitch = 0.0;
    double m_roll = 0.0;
public:
    Camera() = default;
    Camera(const vec3& eye_, const vec3& center_, const vec3& up_, int w = 600, int h = 600,
           double fov_ = 55.0, double near_ = 2.0, double far_ = 3.0);

    [[nodiscard]] int getWidth() const { return m_width; }
    [[nodiscard]] int getHeight() const { return m_height; }
    [[nodiscard]] double getAspect() const { return m_aspect; }
    vec3& getEye() { return m_eye; }

    void updateRotate();
    void updateEuler();
    [[nodiscard]] mat4 get_view_matrix() const;
    [[nodiscard]] mat4 get_projection_matrix() const;
    [[nodiscard]] mat4 get_viewport_matrix() const;

    /**
     *  @brief 设置相机朝向，基于相机位置旋转，顺时针旋转
     *  @param yaw 水平旋转角度，单位度
     *  @param pitch 垂直旋转角度，单位度
     *  @param roll 侧滚旋转角度，单位度
     */
    void set_toward_from_center(double yaw, double pitch, double roll);

    /**
     *  @brief 设置相机朝向，基于目标点旋转，顺时针旋转
     *  @param yaw 水平旋转角度，单位度
     *  @param pitch 垂直旋转角度，单位度
     *  @param roll 侧滚旋转角度，单位度
     */
    void set_toward_from_point(vec3 point, double yaw, double pitch, double roll);

    /**
     *  @brief 设置相机朝向增量，基于相机位置旋转，顺时针旋转
     *  @param yaw 水平旋转角度，单位度
     *  @param pitch 垂直旋转角度，单位度
     *  @param roll 侧滚旋转角度，单位度
     */
    void rotate_around_center(double yaw, double pitch, double roll);

    /**
     *  @brief 设置相机朝向增量，基于目标点旋转，顺时针旋转
     *  @param yaw 水平旋转角度，单位度
     *  @param pitch 垂直旋转角度，单位度
     *  @param roll 侧滚旋转角度，单位度
     */
    void rotate_around_point(vec3 point, double yaw, double pitch, double roll);

    friend class Scene;
    friend class ConfigGui;
};

#endif //MICRORENDERER_CAMERA_H