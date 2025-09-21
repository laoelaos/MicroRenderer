//
// Created by laoe on 25-9-7.
//

#include "Shader.h"

std::vector<std::unique_ptr<TGAImage>> Phong_Shadow_Shader::LightMaps;
vec3 Phong_Shadow_Shader::shade() {
    vec3 result{};
    for (auto light : light_info)
    {
        vec3 light_color = light.get_illumination_at(position);
        vec3 light_dir = normalize(light.position - position);
        vec3 view_dir = normalize(vec3{0, 0, 0} - position);
        vec3 half_vec = normalize(light_dir + view_dir);

        //ambient
        result += properties->k_ambient * vec3{10, 10, 10};
        //diffuse
        double diff = std::max(0., normal * light_dir);
        result += properties->k_diffuse * cwise_multiply(light_color, color) * diff;
        //specular
        double spec = std::pow(std::max(0., normal * half_vec), properties->p);
        result += properties->k_specular * cwise_multiply(light_color, color) * spec;
    }
    return result;
}