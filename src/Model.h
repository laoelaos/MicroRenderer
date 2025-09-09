//
// Created by laoe on 25-9-4.
//

#ifndef MODEL_H
#define MODEL_H
#include <vector>
#include "Shader.h"

struct Material {
    Material(const std::string& texture_path = "",
             const std::string& normal_map_path = "",
             double k_diffuse_ = 0,
             double k_specular_ = 0,
             double k_ambient_ = 0,
             int p = 0) : k_diffuse(k_diffuse_), k_specular(k_specular_), k_ambient(k_ambient_), p(p) {

        if (!texture_path.empty()) {
            if (!texture.read_tga_file(texture_path)) {
                throw std::runtime_error("Failed to load texture: " + texture_path);
            }
            texture.flip_vertically();
        }
        if (!normal_map_path.empty()) {
            if (!normal_map.read_tga_file(normal_map_path)) {
                throw std::runtime_error("Failed to load normal map: " + normal_map_path);
            }
            normal_map.flip_vertically();
        }

    }
    TGAImage texture;
    TGAImage normal_map;

    double k_diffuse, k_specular, k_ambient;
    int p;

    bool smooth_shading = true;
    bool normal_mapping = true;
};

class Model {
public:
    std::vector<triangle> triangles;
    Material material;
    explicit Model(const std::string& filename);
};

#endif //MODEL_H