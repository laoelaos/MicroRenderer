//
// Created by laoe on 25-9-4.
//

#ifndef MODEL_H
#define MODEL_H
#include <array>
#include <vector>

#include "Geometry.h"
#include "Texture.h"

enum NormalMapType {
    NormalMapType_GLOBAL = 0,
    NormalMapType_TANGENT,
    NormalMapType_NOT_USE
};
enum ShadeFrequency {
    ShadeFrequency_FLAT = 0,
    ShadeFrequency_PER_FRAGMENT
};

struct PhongProperties {
    double k_diffuse = 0.8;
    double k_specular = 0.5;
    double k_ambient = 0.1;
    int p = 100;
};

struct Material {
    std::string texture_path;
    int texture_index = -1;
    std::shared_ptr<FlatTexture> texture;
    std::string normal_map_path;
    int normal_map_index;
    std::shared_ptr<FlatTexture> normal_map;

    PhongProperties properties;

    bool diffuse_mapping = false;
    NormalMapType normal_type = NormalMapType_NOT_USE;
    ShadeFrequency shade_frequency = ShadeFrequency_FLAT;

    void load_texture(int i);
    void load_normal_map(int i);
};

// struct Vertex {
//     bool discard = false;
//     vec3 world_pos;
//     vec3 screen_pos;
//     vec3 normal;
//     vec2 tex_coords;
//
//     Vertex() = default;
//     Vertex(const vec3& pos, const vec3& normal, const vec2& tex_coords)
//         : world_pos(pos), normal(normal), tex_coords(tex_coords) {}
// };
//
// struct Primitive {
//     bool discard = false;
//     bool frontFacing = true;
//     size_t indices[3] = {0, 0, 0};
// };

enum DefaultMesh {
    DefaultMesh_CUBE = 0,
    DefaultMesh_SPHERE,
    DefaultMesh_PLANE
};

struct Triangle {
    bool discard = false;
    std::array<vec3, 3> world_vertices;
    std::array<vec4, 3> clip_vertices;   // clip-space positions (before perspective divide)
    std::array<vec3, 3> screen_vertices;
    std::array<vec3, 3> normals;
    std::array<vec2, 3> tex_coords;

    Triangle() = default;
    Triangle(const std::array<vec3, 3>& vertices,
             const std::array<vec3, 3>& normals,
             const std::array<vec2, 3>& tex_coords)
        : world_vertices(vertices), normals(normals), tex_coords(tex_coords) {}

    void ProcessVertices(const mat4& MVP, const mat4& MV, const mat4& MVit);
    void ProcessViewport(const mat4& Viewport);

    void set_backface();
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

class Mesh {
public:
    std::vector<Triangle> triangles = {};
    vec3 translation = {0, 0, 0};
    vec3 rotation = {0, 0, 0};
    vec3 scale = {1, 1, 1};

    Mesh() = default;
    explicit Mesh( DefaultMesh type );
    explicit Mesh( const std::string& filename );

    void LoadFromFile(const std::string& filename);

    [[nodiscard]] mat4 GetTransformMatrix() const;

    void ProcessTransform(const mat4& MVP, const mat4& MV, const mat4& MVit);
    void ProcessClipping();
    void ProcessViewport(const mat4& Viewport);
    void ProcessFaceCulling();
};

class Model {
public:
    std::string name = "newObj";
    bool enable = true;
    bool necessary = false;
    std::string model_path;

    Material material;
    std::shared_ptr<Mesh> mesh;

    Model() = default;
    explicit Model( DefaultMesh type );
    explicit Model( const std::string& filename );
};

#endif //MODEL_H

