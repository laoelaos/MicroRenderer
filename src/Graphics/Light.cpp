//
// Created by laoe on 2025/10/12.
//

#include "Light.h"

Light::Light(const vec3& color_, const vec3& position_, double intensity_)
: color(color_), position(position_), intensity(intensity_) {}

Light::Light(const vec3 &color_, const vec3 &position_, double intensity_, Camera &lightCamera)
    : color(color_), position(position_), intensity(intensity_) {
    dirInfo = DirectionalLightInfo();
    dirInfo->LightCamera = lightCamera;
    m_type = DIRECTIONAL_LIGHT;
}

vec3 Light::get_illumination_at(const vec3& point) const {
    return color * intensity / norm2(point - position);
}

void Light::setPosition(const vec3 &position_) {
    if (m_type == POINT_LIGHT) {
        position = position_;
    } else if (m_type == DIRECTIONAL_LIGHT) {

    }
}

void Light::setType(LIGHT_TYPE t) {
    m_type = t;
    if (t == DIRECTIONAL_LIGHT) {
        if (!dirInfo) dirInfo.emplace();
    } else {
        dirInfo.reset();
    }
}
