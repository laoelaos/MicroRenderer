//
// Created by laoe on 25-9-7.
//

#ifndef SHADER_H
#define SHADER_H

#include <optional>

#include "geometry.h"
#include "tgaimage.h"

struct light {
    vec3 position;
    vec3 intensity;
};

struct shader_payload {
    vec3 position;
    vec3 normal;
    vec3 eye_pos;
    vec2 tex_coords;

    std::optional<light> light_info;
    std::optional<vec3> color;
    std::optional<TGAImage> texture;
};

class Shader {
public:
    virtual ~Shader() = default;
    virtual vec3 shade(const shader_payload& payload);
};

#endif //SHADER_H
