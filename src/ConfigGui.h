//
// Created by laoe on 2025/10/14.
//

#ifndef MICRORENDERER_CONFIGGUI_H
#define MICRORENDERER_CONFIGGUI_H

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Scene.h"

class ConfigGui {
public:
    static void ConfigLight(Light& light);
};


#endif //MICRORENDERER_CONFIGGUI_H