//
// Created by laoe on 2025/10/12.
//

#ifndef MICRORENDERER_LIGHT_H
#define MICRORENDERER_LIGHT_H

#include <optional>

#include "Camera.h"
#include "Geometry.h"

class Light {
public:
    vec3 color = {1.0, 1.0, 1.0};
    vec3 position = {20.0, 20.0, 20.0};
    double intensity = 2000.0;

    //TODO: 是否需要新建一个Light类？LightCamera是否与Camera不同
    //TODO: LightCamera与Light移动不同步
    std::optional<Camera> LightCamera;

    Light() = default;
    Light(const vec3& color_, const vec3& position_, double intensity_);

    /** @brief 计算光源在某点的照明强度，遵循反平方衰减定律 */
    vec3 get_illumination_at(const vec3& point) const;
};

#endif //MICRORENDERER_LIGHT_H