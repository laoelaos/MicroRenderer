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

struct DirectionalLightInfo {
    bool haveShadow = true;
    bool lightMove = true;

    Camera LightCamera;
    TGAImage shadowMap;
    mat4 LightN;
};

class Light {
    LIGHT_TYPE m_type = POINT_LIGHT;
public:
    // basic
    vec3 color = {1.0, 1.0, 1.0};
    vec3 position = {20.0, 20.0, 20.0};
    double intensity = 2000.0;

    // shadow / light-camera (for directional light)
    std::optional<DirectionalLightInfo> dirInfo;

    Light() = default;
    Light(const vec3& color_, const vec3& position_, double intensity_);
    Light(const vec3& color_, const vec3& position_, double intensity_, Camera& lightCamera);

    void setPosition(const vec3& position_);

    [[nodiscard]] LIGHT_TYPE getType() const { return m_type; }
    void setType(LIGHT_TYPE t);

    /** @brief 计算光源在某点的照明强度，遵循反平方衰减定律 */
    [[nodiscard]] vec3 get_illumination_at(const vec3& point) const;
};

#endif //MICRORENDERER_LIGHT_H
