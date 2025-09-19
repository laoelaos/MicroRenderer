//
// Created by laoe on 25-9-4.
//

#ifndef MODEL_H
#define MODEL_H
#include <vector>
#include "Shader.h"

enum NormalType {GLOBAL, TANGENT, NONE};
enum ShadeFrequency {FLAT, PER_VERTEX, PER_FRAGMENT};

struct Material {
    explicit Material(const std::string& texture_path = "",
             const std::string& normal_map_path = "",
             const phong_properties &properties = {},
             bool diffuse_mapping = false,
             NormalType normal_type = NONE,
             ShadeFrequency shade_frequency = FLAT)
                    : properties(properties), diffuse_mapping(diffuse_mapping),
                      normal_type(normal_type), shade_frequency(shade_frequency) {

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

    bool diffuse_mapping;
    NormalType normal_type;
    ShadeFrequency shade_frequency;
};

class Model {
public:
    std::vector<triangle> triangles;
    Material material;
    mat4 transform = identity_matrix<4>();
    explicit Model(const std::string& filename);
};

#endif //MODEL_H