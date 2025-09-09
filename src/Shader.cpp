//
// Created by laoe on 25-9-7.
//

#include "Shader.h"
#include "Util.h"

vec3 PhongShader_Texture::shade(const shader_payload &payload) {
    const double k_ambient  = payload.properties.k_ambient;
    const double k_diffuse  = payload.properties.k_diffuse;
    const double k_specular = payload.properties.k_specular;
    const int p = payload.properties.p;

    vec3 color = get_color_vec3_from_tga(payload.texture, payload.tex_coords);

    vec3 result{};
    for (auto [position, intensity] : payload.light_info)
    {
        vec3 light_dir = normalize(position - payload.position);
        vec3 view_dir = normalize(vec3{0, 0, 0} - payload.position);
        vec3 half_vec = normalize(light_dir + view_dir);

        //ambient
        result += k_ambient * vec3{10, 10, 10};
        //diffuse
        double r2 = norm2(position - payload .position);
        double diff = std::max(0., payload.normal * light_dir);
        result += k_diffuse * cwise_multiply(intensity, color) / r2 * diff;
        //specular
        double spec = std::pow(std::max(0., payload.normal * half_vec), p);
        result += k_specular * cwise_multiply(intensity, color) / r2 * spec;
    }

    return result;
}

vec3 PhongShader_Color::shade(const shader_payload &payload) {
    const vec3 k_ambient  {0.005, 0.005, 0.005};
    const vec3 k_diffuse  {0.6, 0.6, 0.6};
    const vec3 k_specular {0.3, 0.3, 0.3};
    const int p = 150;

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
