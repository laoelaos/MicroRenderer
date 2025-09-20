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

        load_texture(texture_path);
        load_normal_map(normal_map_path);
    }
    std::string texture_path;
    TGAImage texture;
    std::string normal_map_path;
    TGAImage normal_map;

    phong_properties properties;

    bool diffuse_mapping;
    NormalType normal_type;
    ShadeFrequency shade_frequency;

    void load_texture();
    void load_texture(const std::string& texture_path);
    void load_normal_map();
    void load_normal_map(const std::string& normal_map_path);
};

struct triangle {
    std::array<vec3, 3> vertices;
    std::array<vec3, 3> normals;
    std::array<vec2, 3> tex_coords;
};

class Model {
public:
    std::string name = "newObj";
    bool enable = false;

    std::string model_path;
    std::vector<triangle> triangles = {};

    Material material;
    vec3 translation = {0, 0, 0};
    vec3 rotation = {0, 0, 0};
    vec3 scale = {1, 1, 1};

    Model() = default;
    explicit Model( const std::string& filename );

    void load_obj();
    void load_obj(const std::string& filename);
    [[nodiscard]] mat4 get_transform_matrix() const;
};

#endif //MODEL_H