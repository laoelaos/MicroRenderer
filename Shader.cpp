//
// Created by laoe on 25-9-7.
//

#include "Shader.h"

class PhongShader : public Shader {
    vec3 shade(const shader_payload &payload) override {
        constexpr vec3 k_ambient  {0.005, 0.005, 0.005};
        constexpr vec3 k_diffuse  {0.6, 0.6, 0.6};
        constexpr vec3 k_specular {0.3, 0.3, 0.3};
        constexpr int p = 150;

        vec3 color = vec3(payload.texture.get(static_cast<int>(payload.tex_coords.x), static_cast<int>(payload.tex_coords.y))) / 255.0;

        vec3 result{};
        for (auto [position, intensity] : payload.light_info)
        {
            //ambient
            result += cwise_multiply(k_ambient, intensity);
            //diffuse
            double r2 = norm2(position - payload.position);
            vec3 light_dir = normalize(position - payload.position);
            double diff = std::max(0., payload.normal * light_dir);
            result += cwise_multiply(k_diffuse, intensity) / r2 * diff;
            //specular
            vec3 view_dir = normalize(payload.eye_pos - payload.position);
            vec3 half_vec = normalize(light_dir + view_dir);
            double spec = std::pow(std::max(0., payload.normal * half_vec), p);
            result += cwise_multiply(k_specular, intensity) / r2 * spec;
        }

        return result;
    }
};
