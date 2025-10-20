//
// Created by laoe on 2025/10/12.
//

#ifndef MICRORENDERER_LIGHT_H
#define MICRORENDERER_LIGHT_H

#include <optional>

#include "Camera.h"
#include "Geometry.h"
#include "Model.h"

class Scene;

enum LightType {
    LightType_POINT = 0,
    LightType_DIRECTIONAL
};

struct PointLightInfo {
    std::shared_ptr<SixFacesCubeTexture> shadowMap;
    mat4 LightN[6];
};

struct DirectionalLightInfo {
    Camera LightCamera;
    std::shared_ptr<FlatTexture> shadowMap;
    mat4 LightN;
};

class Light {
    bool m_haveShadow = true;
    bool m_lightMove = true;
    LightType m_type = LightType_POINT;

    // basic
    vec3 m_color = {1.0, 1.0, 1.0};
    vec3 m_position = {20.0, 20.0, 20.0};
    double m_intensity = 2000.0;

    // model
    std::shared_ptr<Mesh> m_lightMesh;

    // shadow / light-sphere (for point light)
    std::optional<PointLightInfo> m_pointInfo;

    // shadow / light-camera (for directional light)
    std::optional<DirectionalLightInfo> m_dirInfo;
public:
    Light();
    Light(const vec3& color_, const vec3& position_, double intensity_);
    Light(const vec3& color_, const vec3& position_, double intensity_, Camera& lightCamera);

    [[nodiscard]] vec3 getPosition() const { return m_position; }
    void setPosition(const vec3& position_);

    [[nodiscard]] LightType getType() const { return m_type; }
    void setType(LightType t);

    void ProcessShadowMapIfNeeded(Scene& scene);
    [[nodiscard]] float getVisibility(const vec4& world_pos) const;

    /** @brief 计算光源在某点的照明强度，遵循反平方衰减定律 */
    [[nodiscard]] vec3 get_illumination_at(const vec3& point) const;

    friend class Scene;
    friend class ConfigGui;
};

#endif //MICRORENDERER_LIGHT_H
