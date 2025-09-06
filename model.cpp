//
// Created by laoe on 25-9-4.
//

#include <fstream>
#include <sstream>
#include "model.h"

Model::Model(std::string filename) {
    std::ifstream file(filename);

    std::string line;
    std::string flag;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        iss >> flag;

        if (flag == "v") {
            double x, y, z;
            iss >> x >> y >> z;
            vertices.push_back({x, y, z});
        } else if (flag == "f") {
            int idx0, idx1, idx2;
            iss >> idx0 >> flag >> idx1 >> flag >> idx2;
            faces.push_back(--idx0);
            faces.push_back(--idx1);
            faces.push_back(--idx2);
        }
    }
}

int Model::getNumberVertex() const {
    return vertices.size();
}

int Model::getNumberFace() const {
    return faces.size() / 3;
}

vec3 Model::getVertex(int index) const {
    return vertices[index];
}

vec3 Model::getVertex(int face_index, int vertex_index) const {
    return vertices[faces[face_index * 3 + vertex_index]];
}
