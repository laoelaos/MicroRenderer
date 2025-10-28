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
    bool loaded = false;
public:
    Camera camera;
    std::vector<Light> lights;
    std::vector<Model> models;

    std::shared_ptr<SingleCubeTexture> skybox_texture;

    Scene() = default;
    explicit Scene(const std::string& filename);

    void SetMove();

    void save_path_file() const;
    void save_path_file(const std::string& filename) const;

    [[nodiscard]] bool is_loaded() const { return loaded; }
private:
    void SaveModel(std::ofstream& file, const Model& model) const;
    void LoadModel(std::ifstream& file);
    void SaveCamera(std::ofstream& file) const;
    void LoadCamera(std::ifstream& file);
    void SaveLight(std::ofstream& file, const Light& light) const;
    void LoadLight(std::ifstream& file, LightType type);

    friend class RenderGui;
};

#endif //MICRORENDERER_SCENE_H