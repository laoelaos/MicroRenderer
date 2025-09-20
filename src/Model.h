//
// Created by laoe on 25-9-4.
//

#ifndef MODEL_H
#define MODEL_H
#include <array>
#include <vector>

#include "Geometry.h"
#include "TGAImage.h"

enum NormalType {GLOBAL, TANGENT, NONE};
enum ShadeFrequency {FLAT, PER_VERTEX, PER_FRAGMENT};

struct phong_properties {
    double k_diffuse;
    double k_specular;
    double k_ambient;
    int p;
};

struct Material {
    explicit Material(const std::string& texture_path = "",
             const std::string& normal_map_path = "",
             const phong_properties &properties = {},
             bool diffuse_mapping = false,
             NormalType normal_type = NONE,
             ShadeFrequency shade_frequency = FLAT)
                    : properties(properties), diffuse_mapping(diffuse_mapping),
                      normal_type(normal_type), shade_frequency(shade_frequency) {

        set_texture(texture_path);
        set_normal_map(normal_map_path);
    }

    TGAImage texture;
    TGAImage normal_map;

    phong_properties properties;

    bool diffuse_mapping;
    NormalType normal_type;
    ShadeFrequency shade_frequency;

    void set_texture(const std::string& texture_path);
    void set_normal_map(const std::string& normal_map_path);
};

struct triangle {
    std::array<vec3, 3> vertices;
    std::array<vec3, 3> normals;
    std::array<vec2, 3> tex_coords;
};

class Model {
public:
    std::vector<triangle> triangles;
    Material material;
    mat4 transform = identity_matrix<4>();
    explicit Model(const std::string& filename);
};

#endif //MODEL_H