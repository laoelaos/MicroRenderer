//
// Created by laoe on 25-9-4.
//

#include <fstream>
#include <sstream>

#include "Model.h"
#include "Geometry.h"

void Material::set_texture(const std::string &texture_path) {
    if (!texture_path.empty()) {
        if (!texture.read_tga_file(texture_path)) {
            throw std::runtime_error("Failed to load texture: " + texture_path);
        }
        texture.flip_vertically();
    }
}

void Material::set_normal_map(const std::string &normal_map_path) {
    if (!normal_map_path.empty()) {
        if (!normal_map.read_tga_file(normal_map_path)) {
            throw std::runtime_error("Failed to load normal map: " + normal_map_path);
        }
        normal_map.flip_vertically();
    }
}

Model::Model(const std::string& filename) {
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
                tri.vertices[cnt] = vertices[i1 - 1];
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
}
