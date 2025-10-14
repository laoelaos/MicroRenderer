//
// Created by laoe on 2025/10/12.
//

#ifndef MICRORENDERER_LIGHT_H
#define MICRORENDERER_LIGHT_H

#include <optional>

#include "Camera.h"
#include "Geometry.h"
#include "../TGAImage.h"

class Scene;

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

    // basic
    vec3 m_color = {1.0, 1.0, 1.0};
    vec3 m_position = {20.0, 20.0, 20.0};
    double m_intensity = 2000.0;

    // shadow / light-camera (for directional light)
    std::optional<DirectionalLightInfo> m_dirInfo;
public:
    Light() = default;
    Light(const vec3& color_, const vec3& position_, double intensity_);
    Light(const vec3& color_, const vec3& position_, double intensity_, Camera& lightCamera);

    [[nodiscard]] vec3 getPosition() const { return m_position; }
    void setPosition(const vec3& position_);

    [[nodiscard]] LIGHT_TYPE getType() const { return m_type; }
    void setType(LIGHT_TYPE t);

    void ProcessShadowMapIfNeeded(Scene& scene);
    [[nodiscard]] float getVisibility(const vec4& world_pos) const;

    /** @brief 计算光源在某点的照明强度，遵循反平方衰减定律 */
    [[nodiscard]] vec3 get_illumination_at(const vec3& point) const;

    friend class Scene;
    friend class ConfigGui;
};

#endif //MICRORENDERER_LIGHT_H
