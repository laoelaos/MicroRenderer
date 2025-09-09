//
// Created by laoe on 25-9-7.
//

#include "Shader.h"
#include "Util.h"

vec3 PhongShader::shade(const shader_payload &payload) {
    const double k_ambient  = payload.properties.k_ambient;
    const double k_diffuse  = payload.properties.k_diffuse;
    const double k_specular = payload.properties.k_specular;
    const int p = payload.properties.p;

    vec3 color = payload.color;

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