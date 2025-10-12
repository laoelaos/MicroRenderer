//
// Created by laoe on 25-9-7.
//

#ifndef SHADER_H
#define SHADER_H

#include <vector>

#include "Light.h"
#include "Model.h"
#include "TGAImage.h"

class Shader {
public:
    std::vector<Light> light_info;
    virtual ~Shader() = default;
    virtual vec3 shade() = 0;
};

//TODO: Phong_Shader类

class Phong_Shadow_Shader : public Shader {
    const double bias = 1e-3; //阴影偏移
public:
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 tex_coords;
    const PhongProperties* properties;

    static vec3 camera_pos;
    static std::vector<TGAImage> LightMaps;
    static std::vector<mat4> LightN;
    static mat4 MainCameraM;

    vec3 shade() override;
};

#endif //SHADER_H
