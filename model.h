//
// Created by laoe on 25-9-4.
//

#ifndef MODEL_H
#define MODEL_H
#include <vector>
#include "geometry.h"
#include "Shader.h"

class Model {
public:
    std::vector<triangle> triangles;
    TGAImage texture;

    explicit Model(const std::string& filename);
};

#endif //MODEL_H