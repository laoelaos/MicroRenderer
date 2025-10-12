//
// Created by laoe on 2025/10/12.
//

#ifndef MICRORENDERER_LIGHT_H
#define MICRORENDERER_LIGHT_H

#include <optional>

#include "Camera.h"
#include "Geometry.h"
#include "../TGAImage.h"

enum LIGHT_TYPE {
    POINT_LIGHT,
    DIRECTIONAL_LIGHT
};

class Light {
public:
    // basic
    vec3 color = {1.0, 1.0, 1.0};
    vec3 position = {20.0, 20.0, 20.0};
    double intensity = 2000.0;

    // kind
    LIGHT_TYPE type = POINT_LIGHT;

    // shadow / light-camera (for directional light)
    std::optional<Camera> LightCamera = std::nullopt;
    bool haveShadow = true;
    bool lightMove = true;
    TGAImage shadowMap;
    mat4 LightN;

    Light() = default;
    Light(const vec3& color_, const vec3& position_, double intensity_);
    virtual ~Light() = default;

    void init_directional_with_camera(const Camera& camera_);

    virtual void setPosition(const vec3& position_) { position = position_; }

    [[nodiscard]] virtual LIGHT_TYPE getType() const { return type; }

    /** @brief 计算光源在某点的照明强度，遵循反平方衰减定律 */
    [[nodiscard]] vec3 get_illumination_at(const vec3& point) const;
};

#endif //MICRORENDERER_LIGHT_H
