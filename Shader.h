//
// Created by laoe on 25-9-7.
//

#ifndef SHADER_H
#define SHADER_H

#include <array>

#include "geometry.h"
#include "tgaimage.h"

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

// information passed to the shader,
// normal should be normalized
struct shader_payload {
    vec3 position;
    vec3 normal;
    vec3 eye_pos;

    std::vector<light> light_info;

    vec2 tex_coords;
    TGAImage texture;
};

class Shader {
public:
    virtual ~Shader() = default;
    virtual vec3 shade(const shader_payload& payload);
};

#endif //SHADER_H
