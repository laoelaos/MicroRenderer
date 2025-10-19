//
// Created by laoe on 2025/10/16.
//

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3_loader.h"
#include "RenderGui.h"

#include <chrono>

#include "ConfigGui.h"
#include "Logger.h"
#include "TextureGui.h"

#include "../Rasterizer.h"

RenderGui::RenderGui() {
    InitTexture();
    Rasterizer::get().set_msaa(m_RenderContext.msaa_level);
}

RenderGui& RenderGui::Get() {
    static RenderGui instance;
    return instance;
}

void RenderGui::LaunchRender() {
    ConfigGui::Get().LaunchConfig(m_Scene);
    TextureGui::Get().LaunchTextureGui();

    ImGui::Begin("渲染设置");
    ConfigBasic();
    ConfigRenderSetting();
    ShowFPS();
    ImGui::End();

    RenderResult();
}

void RenderGui::ConfigBasic() {
    if (ImGui::CollapsingHeader("基本设置")) {
        const char* items[] = { "仅Z测试", "Phong光照", "Phong光照+阴影" };
        int current_mode = m_RenderContext.render_mode;
        if (ImGui::Combo("渲染模式", &current_mode, items, IM_ARRAYSIZE(items))) {
            m_RenderContext.render_mode = static_cast<RasterizerMode>(current_mode);
        }

        if (ImGui::InputInt("MSAA级别", &m_RenderContext.msaa_level, 1, 1)) {
            Rasterizer::get().set_msaa(std::clamp(m_RenderContext.msaa_level, 1, 10));
        }
    }
}

void RenderGui::ConfigRenderSetting() {
    if (ImGui::CollapsingHeader("实时渲染设置", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("启用实时渲染", &m_RenderContext.real_time_rendering);
        ImGui::Checkbox("自动旋转模型", &m_RenderContext.auto_rotate);

        if (m_RenderContext.auto_rotate) {
            ImGui::SliderFloat("旋转速度", &m_RenderContext.rotation_speed, 0.1f, 5.0f, "%.1f");
        }

        // 目标帧率设置
        ImGui::SliderFloat("目标帧率", &m_RenderContext.target_fps, 10.0f, 120.0f, "%.1f");

        int refresh_interval = static_cast<int>(m_RenderContext.refresh_interval);
        ImGui::InputInt("信息刷新间隔(ms)", &refresh_interval);
        m_RenderContext.refresh_interval = std::max(100, refresh_interval);
    }

    if (ImGui::Button("强制重新渲染")) {
        m_RenderContext.force_render = true;
    }
}

void RenderGui::ShowFPS() {
    ImGui::Text("渲染纹理: ID=%u, 尺寸=%dx%d", m_TextureID,
                m_TextureBuffer->GetWidth(), m_TextureBuffer->GetHeight());
    ImGui::Text("渲染时间: %lld ms | FPS: %.1f", m_RenderContext.avg_render_time, m_RenderContext.current_fps);

    if (m_RenderContext.real_time_rendering || m_RenderContext.auto_rotate) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "实时渲染已启用");
    } else {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "按需渲染模式");
    }
}

void RenderGui::RenderResult() {
    ImGui::Begin("渲染结果");

    if (m_RenderContext.force_render ||
        m_RenderContext.real_time_rendering ||
        m_RenderContext.auto_rotate) {
        PerformRendering();
        LoadTexture();
    }

    if (m_RenderContext.force_render) {
        m_RenderContext.force_render = false;
        m_RenderContext.current_fps = 0.0f;
    }

    // 获取窗口内容区域大小以调整图像大小
    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    auto aspectRatio = static_cast<float>(m_Scene.camera.getAspect());

    // 显示纹理，保持纵横比
    ImVec2 imageSize;
    if (windowSize.x / aspectRatio <= windowSize.y) {
        imageSize = ImVec2(windowSize.x, windowSize.x / aspectRatio);
    } else {
        imageSize = ImVec2(windowSize.y * aspectRatio, windowSize.y);
    }

    // 居中显示图像
    float offsetX = (windowSize.x - imageSize.x) * 0.5f;
    float offsetY = (windowSize.y - imageSize.y) * 0.5f;
    if (offsetX > 0 || offsetY > 0) {
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY));
    }

    ImGui::Image(m_TextureID, imageSize);
    ImGui::End();
}

void RenderGui::PerformRendering() {
    auto start_time = std::chrono::high_resolution_clock::now();

    if (m_RenderContext.auto_rotate) {
        double delta_time = 1.0 / m_RenderContext.target_fps; // 估算的时间间隔
        m_Scene.camera.rotate_around_point(vec3{0, 0, 0}, m_RenderContext.rotation_speed * delta_time * 60.0, 0, 0);
    }

    FrameBuffer fb;
    if (m_RenderContext.render_mode == ZTEST) {
        fb = {nullptr, m_TextureBuffer};
        Rasterizer::get().pass(m_Scene, ZTEST, fb);
    } else if (m_RenderContext.render_mode == PHONG) {
        fb = {m_TextureBuffer, nullptr};
        Rasterizer::get().pass(m_Scene, PHONG, fb);
    } else if (m_RenderContext.render_mode == PHONG_WITH_SHADOW) {
        fb = {m_TextureBuffer, nullptr};
        for (Light& light : m_Scene.lights) {
            light.ProcessShadowMapIfNeeded(m_Scene);
        }
        Rasterizer::get().pass(m_Scene, PHONG_WITH_SHADOW, fb);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    static int frame_counter = 0;
    static long long duration_counter = 0;
    frame_counter++;
    duration_counter += duration;
    if (duration_counter >= m_RenderContext.refresh_interval) {
        m_RenderContext.avg_render_time = duration_counter / frame_counter;
        m_RenderContext.current_fps = 1000.0f / static_cast<float>(m_RenderContext.avg_render_time);
        frame_counter = 0;
        duration_counter = 0;
    }
}

void RenderGui::InitTexture() {
    LOGI("RenderGui::InitTexture: Initializing texture buffer (%dx%d)",
         m_Scene.camera.getWidth(), m_Scene.camera.getHeight());

    if (m_Scene.camera.getWidth() <= 0 || m_Scene.camera.getHeight() <= 0) {
        LOGE("RenderGui::InitTexture: Invalid texture dimensions: %dx%d",
             m_Scene.camera.getWidth(), m_Scene.camera.getHeight());
        return;
    }

    m_TextureBuffer = std::make_shared<Buffer<RGBA>>(m_Scene.camera.getWidth(), m_Scene.camera.getHeight());
    glGenTextures(1, &m_TextureID);
    glBindTexture(GL_TEXTURE_2D, m_TextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    LOGI("RenderGui::InitTexture: Texture initialized successfully (ID: %u)", m_TextureID);
}

void RenderGui::LoadTexture() {
    if (!m_TextureBuffer) {
        LOGE("RenderGui::LoadTexture: Texture buffer is null");
        return;
    }

    int width = m_TextureBuffer->GetWidth();
    int height = m_TextureBuffer->GetHeight();

    auto* data = m_TextureBuffer->GetP(0, 0);

    // 翻转图像数据
    std::vector<RGBA> flipped_data(width * height);
    for (int y = 0; y < height; y++) {
        memcpy(flipped_data.data() + (height - 1 - y) * width, data + y * width, width * sizeof(RGBA));
    }

    glBindTexture(GL_TEXTURE_2D, m_TextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, flipped_data.data());
}
