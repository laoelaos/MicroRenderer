//
// Created by laoe on 25-9-4.
//

#ifndef MODEL_H
#define MODEL_H
#include <vector>
#include "Shader.h"

struct Material {
    explicit Material(const std::string& texture_path = "",
             const std::string& normal_map_path = "",
             const phong_properties &properties = {}) : properties(properties) {

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

    phong_properties properties;

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