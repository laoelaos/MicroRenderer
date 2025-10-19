//
// Created by laoe on 2025/10/16.
//

#ifndef MICRORENDERER_RENDERGUI_H
#define MICRORENDERER_RENDERGUI_H

#include "Rasterizer.h"
#include "Scene.h"
#include "Util/Buffer.h"

struct RenderContext {
    int msaa_level = 1;
    RasterizerMode render_mode = PHONG_WITH_SHADOW;

    // 渲染选项
    bool real_time_rendering = true;    // 是否启用实时渲染
    bool auto_rotate = false;           // 是否自动旋转模型
    float rotation_speed = 1.0f;        // 旋转速度
    double current_rotation = 0.0;      // 当前旋转角度

    // 性能监控
    float target_fps = 60.0f;           // 目标帧率
    float current_fps = 0.0f;           // 当前帧率
    long long avg_render_time = 0;     // 上次渲染耗时(ms)
    long long refresh_interval = 500;  // 刷新间隔(ms)

    // 控制标志
    bool force_render = true;
};

class RenderGui {
    Scene m_Scene = Scene("../obj/default2.sc");

    RenderContext m_RenderContext;

    uint32_t m_TextureID = 0;
    std::shared_ptr<Buffer<RGBA>> m_TextureBuffer;

    bool m_Save = false;

    RenderGui();
public:
    static RenderGui& get();

    RenderContext& getSettings() { return m_RenderContext; }

    void LaunchRender();
private:
    void ConfigBasic();
    void ConfigRenderSetting();
    void ShowFPS();
    void RenderResult();

    void PerformRendering();
    void InitTexture();
    void LoadTexture();
};


#endif //MICRORENDERER_RENDERGUI_H

