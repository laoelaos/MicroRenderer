//
// Created by laoe on 25-9-4.
//

#ifndef MODEL_H
#define MODEL_H
#include <array>
#include <vector>

#include "Geometry.h"
#include "../TGAImage.h"

enum NormalType {GLOBAL, TANGENT, NONE};
enum ShadeFrequency {FLAT, PER_VERTEX, PER_FRAGMENT};

struct PhongProperties {
    double k_diffuse;
    double k_specular;
    double k_ambient;
    int p;
};

struct Material {
    explicit Material(const std::string& texture_path = "",
             const std::string& normal_map_path = "",
             const PhongProperties &properties = {},
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

    PhongProperties properties;

    bool diffuse_mapping;
    NormalType normal_type;
    ShadeFrequency shade_frequency;

    void load_texture();
    void load_texture(const std::string& texture_path);
    void load_normal_map();
    void load_normal_map(const std::string& normal_map_path);
};

struct Triangle {
    std::array<vec3, 3> world_vertices;
    std::array<vec3, 3> screen_vertices;
    std::array<vec3, 3> normals;
    std::array<vec2, 3> tex_coords;

    void get_vertices(const mat4& mvpv, const mat4& mv);
    void get_normal(const mat4& mvit);

    bool is_backface() const;
    bool is_invalid() const;

    void get_barycentric(double x, double y);
    void get_barycentric_correct(double x, double y);

    vec3 get_interpolated_normal() const;
    vec3 get_interpolated_world_position() const;
    vec2 get_interpolated_tex_coords() const;
    double get_interpolated_z() const;

    [[nodiscard]] std::tuple<int, int, int, int> find_bounding_box_int(int width, int height) const;
private:
    double alpha = 0, beta = 0, gamma = 0;
    double c_alpha = 0, c_beta = 0, c_gamma = 0;
};

class Model {
public:
    std::string name = "newObj";
    bool enable = false;

    std::string model_path;
    std::vector<Triangle> triangles = {};

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