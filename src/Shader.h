//
// Created by laoe on 25-9-7.
//

#ifndef SHADER_H
#define SHADER_H

#include <array>

#include "Geometry.h"
#include "TGAImage.h"

//vertices, normals, colors and texture coordinates (std::array<vec3, 3>)
struct triangle {
    std::array<vec3, 3> vertices;
    std::array<vec3, 3> normals;

    std::array<vec3, 3> colors;
    std::array<vec2, 3> tex_coords;
};

struct light {
    vec3 position;
    vec3 intensity;
};

struct phong_properties {
    double k_diffuse;
    double k_specular;
    double k_ambient;
    int p;
};

// position, normal, light_info, tex_coords, texture
//
// position is in world space
// normal should be normalized
struct shader_payload {
    vec3 position;
    vec3 normal;
    vec2 tex_coords;

    phong_properties properties;
    std::vector<light> light_info;
    TGAImage texture;
};

class Shader {
public:
    virtual ~Shader() = default;
    virtual vec3 shade(const shader_payload& payload) = 0;
};

class PhongShader_Texture : public Shader {
public:
    vec3 shade(const shader_payload &payload) override;
};

class PhongShader_Color : public Shader {
public:
    vec3 shade(const shader_payload &payload) override;
};

#endif //SHADER_H
