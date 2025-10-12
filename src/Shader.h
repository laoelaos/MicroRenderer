//
// Created by laoe on 25-9-7.
//

#ifndef SHADER_H
#define SHADER_H

#include <vector>

#include "Light.h"
#include "Model.h"

class Shader {
public:
    static const std::vector<Light>* s_lightInfo;
    static std::vector<vec3> s_lightPos;
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

    static mat4 s_mainCameraM;

    vec3 shade() override;
};

#endif //SHADER_H
