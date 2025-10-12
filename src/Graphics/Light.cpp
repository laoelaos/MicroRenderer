//
// Created by laoe on 2025/10/12.
//

#include "Light.h"

Light::Light(const vec3& color_, const vec3& position_, double intensity_)
: color(color_), position(position_), intensity(intensity_) {}

vec3 Light::get_illumination_at(const vec3& point) const {
    return color * intensity / norm2(point - position);
}