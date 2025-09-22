//
// Created by laoe on 25-9-7.
//

#include "Shader.h"

#include <iostream>

vec3 Phong_Shadow_Shader::camera_pos;
std::vector<TGAImage> Phong_Shadow_Shader::LightMaps;
std::vector<mat4> Phong_Shadow_Shader::LightN;
mat4 Phong_Shadow_Shader::MainCameraM;

vec3 Phong_Shadow_Shader::shade() {
    vec3 result{};
    for (int i = 0; i < static_cast<int>(light_info.size()); i++)
    {
        Light& light = light_info[i];

        vec3 light_color = light.get_illumination_at(position);
        vec3 light_dir = normalize(light.position - position);
        vec3 view_dir = normalize(camera_pos - position);
        vec3 half_vec = normalize(light_dir + view_dir);

        double visibility;
        vec3 light_space_pos = (LightN[i] * MainCameraM * position.to_vec4(1.0)).to_vec3_point();
        double z = light_space_pos.z;
        if (LightMaps[i].get(static_cast<int>(light_space_pos.x), static_cast<int>(light_space_pos.y)).to_double() > z + bias) {
            visibility = 0.0;
        } else {
            visibility = 1.0;
        }

        //ambient
        result += properties->k_ambient * vec3{10, 10, 10};
        //diffuse
        double diff = std::max(0., normal * light_dir);
        result += properties->k_diffuse * cwise_multiply(light_color, color) * diff * visibility;
        //specular
        double spec = std::pow(std::max(0., normal * half_vec), properties->p);
        result += properties->k_specular * cwise_multiply(light_color, color) * spec * visibility;
    }
    return result;
}