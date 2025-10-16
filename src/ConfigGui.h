//
// Created by laoe on 2025/10/14.
//

#ifndef MICRORENDERER_CONFIGGUI_H
#define MICRORENDERER_CONFIGGUI_H

#include "Scene.h"

class ConfigGui {
    int m_SelectedModel = 0;
    ConfigGui() = default;
public:
    static ConfigGui& get();

    void LaunchConfig(Scene &scene);
private:
    bool ConfigCamera(Camera& camera);
    void ConfigLight(Light& light);
    void ConfigModel(Model& model);
};


#endif //MICRORENDERER_CONFIGGUI_H