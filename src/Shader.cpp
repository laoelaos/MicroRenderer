//
// Created by laoe on 25-9-7.
//

#include "Shader.h"

#include <iostream>

bool PhongShader::s_EnableShadow;
const std::vector<Light>* Shader::s_lightInfo;
std::vector<vec3> Shader::s_lightPos;
mat4 PhongShader::s_mainCameraM;

vec3 PhongShader::shade() {
    vec3 result{};
    for (int i = 0; i < static_cast<int>(s_lightInfo->size()); i++) {
        const Light& light = (*s_lightInfo)[i];

        vec3 light_color = light.get_illumination_at(viewWorldPos);
        vec3 light_dir = normalize(s_lightPos[i] - viewWorldPos);
        vec3 view_dir = normalize(vec3{0, 0, 0} - viewWorldPos);
        vec3 half_vec = normalize(light_dir + view_dir);

        double visibility = 1.0;
        if (s_EnableShadow) {
            visibility = light.getVisibility((s_mainCameraM * viewWorldPos.to_vec4(1.0)));
        }

        //ambient
        result += properties->k_ambient * cwise_multiply(vec3{10, 10, 10}, color);
        //diffuse
        double diff = std::max(0., normal * light_dir);
        result += properties->k_diffuse * cwise_multiply(light_color, color) * diff * visibility;
        //specular
        double spec = std::pow(std::max(0., normal * half_vec), properties->p);
        result += properties->k_specular * cwise_multiply(light_color, color) * spec * visibility;
    }
    return result;
}