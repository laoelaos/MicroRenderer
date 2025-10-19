//
// Created by laoe on 2025/10/14.
//

#ifndef MICRORENDERER_CONFIGGUI_H
#define MICRORENDERER_CONFIGGUI_H

#include "Scene.h"

class ConfigGui {
    int m_SelectedModel = 0;
    int m_DefaultModel = -1;
    ConfigGui() = default;
public:
    static ConfigGui& Get();

    void LaunchConfig(Scene &scene);
private:
    bool ConfigCamera(Camera& camera);
    void ConfigLight(Light& light);
    bool ConfigModel(Model& model);
};


#endif //MICRORENDERER_CONFIGGUI_H