//
// Created by laoe on 25-9-7.
//

#ifndef SHADER_H
#define SHADER_H

#include <memory>
#include "Geometry.h"
#include "Graphics.h"

class Shader {
public:
    std::vector<Light> light_info;
    virtual ~Shader() = default;
    virtual vec3 shade() = 0;
};

//TODO: Phong_Shader类

class Phong_Shadow_Shader : public Shader {
public:
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 tex_coords;
    const phong_properties* properties;

    static std::vector<std::unique_ptr<TGAImage>> LightMaps;
    static std::vector<mat4> LightN;
    static mat4 model;

    vec3 shade() override;
};

#endif //SHADER_H
