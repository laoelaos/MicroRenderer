//
// Created by laoe on 25-9-7.
//

#ifndef SHADER_H
#define SHADER_H

#include "Geometry.h"
#include "Graphics.h"

// position, normal, light_info, tex_coords, texture
//
// position is in world space
// normal should be normalized
struct shader_payload {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 tex_coords;

    phong_properties properties;
    std::vector<Light> light_info;
};

class Shader {
public:
    virtual ~Shader() = default;
    virtual vec3 shade(const shader_payload& payload) = 0;
};

class PhongShader : public Shader {
public:
    vec3 shade(const shader_payload &payload) override;
};

#endif //SHADER_H
