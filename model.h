//
// Created by laoe on 25-9-4.
//

#ifndef MODEL_H
#define MODEL_H
#include <vector>
#include "geometry.h"

class Model {
public:
    std::vector<vec3> vertices;
    std::vector<int> faces;

    explicit Model(std::string filename);
    int getNumberVertex() const;
    int getNumberFace() const;
    vec3 getVertex(int index) const;
    vec3 getVertex(int face_index, int vertex_index) const;
};

#endif //MODEL_H