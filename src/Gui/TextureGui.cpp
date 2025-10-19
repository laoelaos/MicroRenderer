//
// Created by laoe on 2025/10/19.
//

#include "TextureGui.h"

#include "ImageUtils.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_opengl3_loader.h"
#include "Texture.h"
#include "Logger.h"

TextureGui& TextureGui::Get() {
    static TextureGui instance;
    return instance;
}

std::shared_ptr<Texture> TextureGui::GetTextureByIndex(int index) {
    if (index < 0 || index >= m_TexturesBuffer.size()) {
        LOGE("TextureGui::GetTextureByIndex: Invalid texture index: %d", index);
        return nullptr;
    }
    LOGI("TextureGui::GetTextureByIndex: Returning texture at index %d", index);
    return m_TexturesBuffer[index].texture;
}

void TextureGui::LaunchTextureGui() {
    ImGui::Begin("纹理管理");

    // 加载纹理部分
    if (ImGui::CollapsingHeader("加载新纹理", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("纹理路径", &m_ReadyToLoadPath);
        ImGui::SameLine();
        if (ImGui::Button("加载")) {
            if (!m_ReadyToLoadPath.empty()) {
                LoadTextureFromFile(m_ReadyToLoadPath);
            }
        }
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "示例: ../obj/floor_diffuse.tga");
    }

    ImGui::Separator();

    // 已加载纹理列表
    if (ImGui::CollapsingHeader("已加载纹理", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("纹理数量: %d", static_cast<int>(m_TexturesBuffer.size()));

        if (m_TexturesBuffer.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "暂无已加载纹理");
        } else {
            if (ImGui::BeginTable("TextureTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("预览", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("信息", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < m_TexturesBuffer.size(); i++) {
                    auto& preview = m_TexturesBuffer[i];

                    ImGui::TableNextRow();

                    // 预览列
                    ImGui::TableSetColumnIndex(0);
                    if (preview.textureID != 0) {
                        // 计算预览图大小，保持纵横比
                        float previewSize = 128.0f;
                        float aspect = static_cast<float>(preview.width) / static_cast<float>(preview.height);
                        ImVec2 imageSize;
                        if (aspect > 1.0f) {
                            imageSize = ImVec2(previewSize, previewSize / aspect);
                        } else {
                            imageSize = ImVec2(previewSize * aspect, previewSize);
                        }
                        ImGui::Image(preview.textureID, imageSize);
                    } else {
                        ImGui::Text("无预览");
                    }

                    // 信息列
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("路径: %s", preview.path.c_str());
                    ImGui::Text("尺寸: %d x %d", preview.width, preview.height);
                    ImGui::Text("Index: %d", preview.index);

                    // 操作列
                    ImGui::TableSetColumnIndex(2);
                    ImGui::PushID(i);
                    if (ImGui::Button("删除", ImVec2(70, 0))) {
                        DeleteTexture(i);
                        ImGui::PopID();
                        break;
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }
    }

    ImGui::End();
}

int TextureGui::LoadTextureFromFile(const std::string& path) {
    auto buffer = ImageUtil::ReadImageRGBA(path, true);
    if (!buffer) {
        LOGE("TextureGui::LoadTextureFromFile: Failed to load texture: %s", path.c_str());
        return -1;
    }

    auto flatTexture = std::make_shared<FlatTexture>();
    flatTexture->SetData(buffer);

    TexturePreview preview;
    preview.texture = flatTexture;
    preview.path = path;
    preview.width = buffer->GetWidth();
    preview.height = buffer->GetHeight();
    preview.index = static_cast<int>(m_TexturesBuffer.size());

    CreateGLTexture(preview);

    m_TexturesBuffer.push_back(preview);

    LOGI("TextureGui::LoadTextureFromFile: success-> %s (ID: %u, Size: %dx%d)", path.c_str(), preview.textureID, preview.width, preview.height);
    return preview.index;
}

void TextureGui::DeleteTexture(int index) {
    if (index < 0 || index >= m_TexturesBuffer.size()) {
        LOGE("TextureGui::DeleteTexture: Invalid texture index: %d", index);
        return;
    }

    auto& preview = m_TexturesBuffer[index];
    if (preview.textureID != 0) {
        glDeleteTextures(1, &preview.textureID);
    }
    m_TexturesBuffer.erase(m_TexturesBuffer.begin() + index);

    LOGI("TextureGui::DeleteTexture: success-> index %d deleted", index);
}

void TextureGui::CreateGLTexture(TexturePreview& preview) {
    auto flatTexture = std::dynamic_pointer_cast<FlatTexture>(preview.texture);
    if (!flatTexture) {
        LOGE("TextureGui::CreateGLTexture: Invalid texture type for preview at index %d", preview.index);
        return;
    }

    auto buffer = flatTexture->GetData();
    if (!buffer) {
        LOGE("TextureGui::CreateGLTexture: Texture has no data at index %d", preview.index);
        return;
    }

    glGenTextures(1, &preview.textureID);
    glBindTexture(GL_TEXTURE_2D, preview.textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, preview.width, preview.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, buffer->GetP(0, 0));

    LOGI("OpenGL texture created: ID=%u", preview.textureID);
}
