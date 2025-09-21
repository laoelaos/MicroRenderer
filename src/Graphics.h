//
// Created by laoe on 25-9-19.
//

#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <optional>
#include <vector>

#include "Geometry.h"
#include "Model.h"

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

class Light {
public:
    vec3 color = {1.0, 1.0, 1.0};
    vec3 position = {20.0, 20.0, 20.0};
    double intensity = 2000.0;

    std::optional<Camera> LightCamera;

    Light() = default;
    Light(const vec3& color_, const vec3& position_, double intensity_);

    /** @brief 计算光源在某点的照明强度，遵循反平方衰减定律 */
    vec3 get_illumination_at(const vec3& point) const;
};

class Scene {
    std::string filename;
public:
    Camera camera;
    std::vector<Light> lights;
    std::vector<Model> models;
    bool light_move = false;

    Scene() = default;
    explicit Scene(const std::string& filename);

    void save_path_file() const;
    void save_path_file(const std::string& filename) const;
};

#endif //GRAPHICS_H
