//
// Created by laoe on 25-9-7.
//

#include "Shader.h"
#include "util.h"

vec3 PhongShader_Texture::shade(const shader_payload &payload) {
    const vec3 k_ambient  {0.005, 0.005, 0.005};
    const vec3 k_diffuse  {0.6, 0.6, 0.6};
    const vec3 k_specular {0.3, 0.3, 0.3};
    constexpr int p = 150;

    vec3 color = color_to_vec3(payload.texture.get(static_cast<int>(payload.tex_coords.x * payload.texture.width()),
                                                   payload.texture.height()-1-static_cast<int>(payload.tex_coords.y * payload.texture.height())));

    vec3 result{};
    for (auto [position, intensity] : payload.light_info)
    {
        vec3 light_dir = normalize(position - payload.position);
        vec3 view_dir = normalize(vec3{0, 0, 0} - payload.position);
        vec3 half_vec = normalize(light_dir + view_dir);

        //ambient
        result += cwise_multiply(k_ambient, {10, 10, 10});
        //diffuse
        double r2 = norm2(position - payload .position);
        double diff = std::max(0., payload.normal * light_dir);
        result += cwise_multiply(k_diffuse, intensity, color) / r2 * diff;
        //specular
        double spec = std::pow(std::max(0., payload.normal * half_vec), p);
        result += cwise_multiply(k_specular, intensity, color) / r2 * spec;
    }

    return result;
}

vec3 PhongShader_Color::shade(const shader_payload &payload) {
    const vec3 k_ambient  {0.005, 0.005, 0.005};
    const vec3 k_diffuse  {0.6, 0.6, 0.6};
    const vec3 k_specular {0.3, 0.3, 0.3};
    constexpr int p = 150;

    vec3 result{};
    for (auto [position, intensity] : payload.light_info)
    {
        vec3 light_dir = normalize(position - payload.position);
        vec3 view_dir = normalize(vec3{0, 0, 0} - payload.position);
        vec3 half_vec = normalize(light_dir + view_dir);

        //ambient
        result += cwise_multiply(k_ambient, {10, 10, 10});
        //diffuse
        double r2 = norm2(position - payload.position);
        double diff = std::max(0., payload.normal * light_dir);
        result += cwise_multiply(k_diffuse, intensity) / r2 * diff;
        //specular
        double spec = std::pow(std::max(0., payload.normal * half_vec), p);
        result += cwise_multiply(k_specular, intensity) / r2 * spec;
    }

    return result;
}
