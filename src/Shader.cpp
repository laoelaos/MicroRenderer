//
// Created by laoe on 25-9-7.
//

#include "Shader.h"

#include <iostream>

const std::vector<Light>* Shader::s_lightInfo;
std::vector<vec3> Shader::s_lightPos;
mat4 Phong_Shadow_Shader::s_mainCameraM;

vec3 Phong_Shadow_Shader::shade() {
    vec3 result{};
    for (int i = 0; i < static_cast<int>(s_lightInfo->size()); i++) {
        const Light& light = (*s_lightInfo)[i];

        vec3 light_color = light.get_illumination_at(position);
        vec3 light_dir = normalize(s_lightPos[i] - position);
        vec3 view_dir = normalize(vec3{0, 0, 0} - position);
        vec3 half_vec = normalize(light_dir + view_dir);

        double visibility = 1.0;
        if (light.getType() == DIRECTIONAL_LIGHT && light.LightCamera.has_value() && light.haveShadow) {
            vec3 light_space_pos = (light.LightN * s_mainCameraM * position.to_vec4(1.0)).to_vec3_point();
            double z = light_space_pos.z;
            if (light.shadowMap.get(static_cast<int>(light_space_pos.x), static_cast<int>(light_space_pos.y)).to_double() > z + bias) {
                visibility = 0.0;
            } else {
                visibility = 1.0;
            }
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