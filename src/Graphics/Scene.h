//
// Created by laoe on 2025/10/12.
//

#ifndef MICRORENDERER_SCENE_H
#define MICRORENDERER_SCENE_H

#include <vector>
#include <string>

#include "Camera.h"
#include "Light.h"
#include "Model.h"


class Scene {
    std::string filename;
public:
    Camera camera;
    std::vector<Light> lights;
    std::vector<Model> models;

    Scene() = default;
    explicit Scene(const std::string& filename);

    void save_path_file() const;
    void save_path_file(const std::string& filename) const;
};

#endif //MICRORENDERER_SCENE_H