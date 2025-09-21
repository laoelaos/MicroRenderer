//
// Created by laoe on 25-9-4.
//

#include <fstream>
#include <sstream>

#include "Model.h"

#include <algorithm>

#include "Geometry.h"

void triangle::get_vertices(const mat4& mvpv, const mat4& mv) {
    for (int i = 0; i < 3; i++) {
        screen_vertices[i] = (mvpv * world_vertices[i].to_vec4(1.0)).to_vec3_point();
    }
    for (int i = 0; i < 3; i++) {
        world_vertices[i] = (mv * world_vertices[i].to_vec4(1.0)).to_vec3_point();
    }
}

void triangle::get_normal(const mat4 &mvit) {
    for (int i = 0; i < 3; i++) {
        normals[i] = (mvit * normals[i].to_vec4(1.0)).to_vec3_point();
    }
}

bool triangle::is_backface() const {
    return ((screen_vertices[1] - screen_vertices[0]) ^ (screen_vertices[2] - screen_vertices[1])).z < 0;
}

bool triangle::is_invalid() const {
    return (alpha < 0 || gamma < 0 || beta < 0);
}

void triangle::get_barycentric(double x, double y) {
    double alpha_denominator = - (screen_vertices[0].x - screen_vertices[1].x) * (screen_vertices[2].y - screen_vertices[1].y) +
        (screen_vertices[0].y - screen_vertices[1].y) * (screen_vertices[2].x - screen_vertices[1].x);
    double beta_denominator = - (screen_vertices[1].x - screen_vertices[2].x) * (screen_vertices[0].y - screen_vertices[2].y) +
        (screen_vertices[1].y - screen_vertices[2].y) * (screen_vertices[0].x - screen_vertices[2].x);

    if (std::abs(alpha_denominator) < 1e-8 || std::abs(beta_denominator) < 1e-8) {
        alpha = -1;
        beta = -1;
        gamma = -1;
        return;
    }

    alpha = (- (x - screen_vertices[1].x) * (screen_vertices[2].y - screen_vertices[1].y) + (y - screen_vertices[1].y) * (screen_vertices[2].x - screen_vertices[1].x)) / alpha_denominator;
    beta = (- (x - screen_vertices[2].x) * (screen_vertices[0].y - screen_vertices[2].y) + (y - screen_vertices[2].y) * (screen_vertices[0].x - screen_vertices[2].x)) / beta_denominator;
    gamma = 1. - alpha - beta;
}

void triangle::get_barycentric_correct(double x, double y) {
    get_barycentric(x, y);
    if (alpha < 0 || beta < 0 || gamma < 0)
        return;
    double z0 = 1.0/world_vertices[0].z;
    double z1 = 1.0/world_vertices[1].z;
    double z2 = 1.0/world_vertices[2].z;
    c_alpha = alpha * z0 / (alpha * z0 + beta * z1 + gamma * z2);
    c_beta = beta * z1 / (alpha * z0 + beta * z1 + gamma * z2);
    c_gamma = gamma * z2 / (alpha * z0 + beta * z1 + gamma * z2);
}

vec3 triangle::get_interpolated_normal() const {
    return normalize(normals[0] * c_alpha + normals[1] * c_beta + normals[2] * c_gamma);
}

vec3 triangle::get_interpolated_world_position() const {
    return world_vertices[0] * c_alpha + world_vertices[1] * c_beta + world_vertices[2] * c_gamma;
}

vec2 triangle::get_interpolated_tex_coords() const {
    return tex_coords[0] * c_alpha + tex_coords[1] * c_beta + tex_coords[2] * c_gamma;
}

double triangle::get_interpolated_z() const {
    return screen_vertices[0].z * alpha + screen_vertices[1].z * beta + screen_vertices[2].z * gamma;
}

std::tuple<int, int, int, int> triangle::find_bounding_box_int(int width, int height) const {
    auto [x_min, x_max] = std::minmax({screen_vertices[0].x, screen_vertices[1].x, screen_vertices[2].x});
    auto [y_min, y_max] = std::minmax({screen_vertices[0].y, screen_vertices[1].y, screen_vertices[2].y});
    x_min = std::max(0, static_cast<int>(std::floor(x_min)));
    x_max = std::min(width - 1, static_cast<int>(std::ceil(x_max)));
    y_min = std::max(0, static_cast<int>(std::floor(y_min)));
    y_max = std::min(height - 1, static_cast<int>(std::ceil(y_max)));
    return {x_min, x_max, y_min, y_max};
}

void Material::load_texture() {
    load_texture(texture_path);
}

void Material::load_normal_map() {
    load_normal_map(normal_map_path);
}

void Material::load_texture(const std::string &texture_path) {
    if (!texture_path.empty()) {
        if (!texture.read_tga_file(texture_path)) {
            throw std::runtime_error("Failed to load texture: " + texture_path);
        }
        texture.flip_vertically();
    }
    this->texture_path = texture_path;
}

void Material::load_normal_map(const std::string &normal_map_path) {
    if (!normal_map_path.empty()) {
        if (!normal_map.read_tga_file(normal_map_path)) {
            throw std::runtime_error("Failed to load normal map: " + normal_map_path);
        }
        normal_map.flip_vertically();
    }
    this->normal_map_path = normal_map_path;
}

Model::Model(const std::string& filename) {
    load_obj(filename);
}

void Model::load_obj() {
    load_obj(this->model_path);
}

void Model::load_obj(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line, flag;
    double f1, f2, f3;
    int i1, i2, i3;
    char trash;

    std::vector<vec3> vertices, vertices_normal;
    std::vector<vec2> vertices_texture;
    std::vector<int> faces;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        flag = "";
        iss >> flag;

        if (flag == "v") {
            iss >> f1 >> f2 >> f3;
            vertices.emplace_back(f1, f2, f3);
        } else if (flag == "vt") {
            iss >> f1 >> f2;
            vertices_texture.emplace_back(f1, f2);
        } else if (flag == "vn") {
            iss >> f1 >> f2 >> f3;
            vertices_normal.emplace_back(f1, f2, f3);
        } else if (flag == "f") {
            int cnt = 0;
            triangle tri;
            while (iss >> i1 >> trash >> i2 >> trash >> i3) {
                tri.world_vertices[cnt] = vertices[i1 - 1];
                tri.normals[cnt] = vertices_normal[i3 - 1];
                tri.tex_coords[cnt] = vertices_texture[i2 - 1];
                cnt++;
            }
            if (cnt != 3) {
                throw std::runtime_error("Only triangular faces are supported.");
            }
            triangles.push_back(tri);
        }
    }
    this->model_path = filename;
}

mat4 Model::get_transform_matrix() const {
    double cos_x = cos(rotation.x * M_PI / 180.0);
    double sin_x = sin(rotation.x * M_PI / 180.0);
    double cos_y = cos(rotation.y * M_PI / 180.0);
    double sin_y = sin(rotation.y * M_PI / 180.0);
    double cos_z = cos(rotation.z * M_PI / 180.0);
    double sin_z = sin(rotation.z * M_PI / 180.0);

    // X轴旋转矩阵
    mat4 rot_x = {{
        {1, 0, 0, 0},
        {0, cos_x, -sin_x, 0},
        {0, sin_x, cos_x, 0},
        {0, 0, 0, 1}
    }};

    // Y轴旋转矩阵
    mat4 rot_y = {{
        {cos_y, 0, sin_y, 0},
        {0, 1, 0, 0},
        {-sin_y, 0, cos_y, 0},
        {0, 0, 0, 1}
    }};

    // Z轴旋转矩阵
    mat4 rot_z = {{
        {cos_z, -sin_z, 0, 0},
        {sin_z, cos_z, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    }};

    // 缩放矩阵
    mat4 scale_mat = {{
        {scale.x, 0, 0, 0},
        {0, scale.y, 0, 0},
        {0, 0, scale.z, 0},
        {0, 0, 0, 1}
    }};

    // 平移矩阵
    mat4 trans_mat = identity_matrix<4>();
    trans_mat[0][3] = translation.x;
    trans_mat[1][3] = translation.y;
    trans_mat[2][3] = translation.z;

    // 组合变换: 先缩放，再旋转，最后平移
    mat4 rot_mat = rot_z * rot_y * rot_x;
    return trans_mat * rot_mat * scale_mat;
}
