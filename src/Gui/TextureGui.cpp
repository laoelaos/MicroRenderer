//
// Created by laoe on 2025/10/19.
//

#include "TextureGui.h"

#include <format>

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
    if (index < 0 || static_cast<size_t>(index) >= m_TexturesBuffer.size()) {
        LOGE("TextureGui::GetTextureByIndex: Invalid texture index: {}", index);
        return nullptr;
    }
    return m_TexturesBuffer[index].texture;
}

void TextureGui::LaunchTextureGui() {
    ImGui::Begin("纹理管理");

    // 加载纹理部分
    if (ImGui::CollapsingHeader("加载新纹理", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* textureType[] = { "平面纹理", "球状纹理" };
        static int currentTextureType = 0;
        ImGui::Combo("纹理类型", &currentTextureType, textureType, IM_ARRAYSIZE(textureType));
        ImGui::InputText("纹理路径", &m_ReadyToLoadPath);
        ImGui::SameLine();
        if (ImGui::Button("加载")) {
            if (!m_ReadyToLoadPath.empty()) {
                LoadTextureFromFile(m_ReadyToLoadPath, static_cast<TextureType>(currentTextureType), true);
            }
        }
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "示例: ../obj/floor_diffuse.tga");
    }

    ImGui::Separator();

    // 已加载纹理列表 - 网格布局
    if (ImGui::CollapsingHeader("已加载纹理", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("纹理数量: %d", static_cast<int>(m_TexturesBuffer.size()));

        if (m_TexturesBuffer.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "暂无已加载纹理");
        } else {
            // 缩略图尺寸滑块
            static float thumbnailSize = 120.0f;
            ImGui::SliderFloat("缩略图大小", &thumbnailSize, 64.0f, 256.0f, "%.0f");

            int name_size = thumbnailSize / 9;

            ImGui::Separator();

            // 计算每行可以放置多少个缩略图
            float windowWidth = ImGui::GetContentRegionAvail().x;
            float cellPadding = 8.0f;
            float cellWidth = thumbnailSize + cellPadding * 2;
            int columnsCount = std::max(1, static_cast<int>(windowWidth / cellWidth));

            // 使用子窗口创建可滚动区域
            ImGui::BeginChild("TextureGrid", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

            // 遍历所有纹理，以网格形式显示
            for (size_t i = 0; i < m_TexturesBuffer.size(); i++) {
                auto& preview = m_TexturesBuffer[i];

                ImGui::PushID(static_cast<int>(i));

                // 开始一个组来包含缩略图和信息
                ImGui::BeginGroup();

                // 绘制带边框的背景
                ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                ImVec2 rectMin = cursorPos;
                ImVec2 rectMax = ImVec2(cursorPos.x + thumbnailSize, cursorPos.y + thumbnailSize + 60);

                // 创建一个不可见的按钮来接收交互（用于右键菜单）
                ImGui::SetCursorScreenPos(rectMin);
                ImGui::InvisibleButton("##thumbnail", ImVec2(thumbnailSize, thumbnailSize + 60));
                bool isHovered = ImGui::IsItemHovered();

                // 绘制背景和边框
                ImU32 bgColor = isHovered ? IM_COL32(60, 60, 80, 255) : IM_COL32(40, 40, 50, 255);
                ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, bgColor, 4.0f);
                ImGui::GetWindowDrawList()->AddRect(rectMin, rectMax, IM_COL32(100, 100, 120, 255), 4.0f, 0, 1.5f);

                // 显示纹理预览
                if (preview.textureID != 0) {
                    float aspect = static_cast<float>(preview.width) / static_cast<float>(preview.height);
                    float imageSize = thumbnailSize - cellPadding * 2;
                    ImVec2 imageDisplaySize;
                    ImVec2 imagePadding(0, 0);

                    if (aspect > 1.0f) {
                        imageDisplaySize = ImVec2(imageSize, imageSize / aspect);
                        imagePadding.y = (imageSize - imageDisplaySize.y) * 0.5f;
                    } else {
                        imageDisplaySize = ImVec2(imageSize * aspect, imageSize);
                        imagePadding.x = (imageSize - imageDisplaySize.x) * 0.5f;
                    }

                    ImVec2 imagePos = ImVec2(cursorPos.x + cellPadding + imagePadding.x,
                                             cursorPos.y + cellPadding + imagePadding.y);
                    ImGui::GetWindowDrawList()->AddImage(
                        preview.textureID,
                        imagePos,
                        ImVec2(imagePos.x + imageDisplaySize.x, imagePos.y + imageDisplaySize.y)
                    );
                } else {
                    // 无纹理时显示占位符
                    ImVec2 textPos = ImVec2(cursorPos.x + cellPadding,
                                           cursorPos.y + cellPadding + (thumbnailSize - cellPadding * 2 - 20) * 0.5f);
                    ImGui::GetWindowDrawList()->AddText(textPos, IM_COL32(255, 128, 128, 255), "无预览");
                }

                // 显示纹理信息（使用 DrawList 直接绘制文本）
                ImVec2 textPos1 = ImVec2(cursorPos.x + cellPadding, cursorPos.y + thumbnailSize - cellPadding);
                ImVec2 textPos2 = ImVec2(cursorPos.x + cellPadding, cursorPos.y + thumbnailSize + 14);
                ImVec2 textPos3 = ImVec2(cursorPos.x + cellPadding, cursorPos.y + thumbnailSize + 28);

                // 提取文件名（去掉路径）
                std::string filename = preview.path;
                size_t lastSlash = filename.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    filename = filename.substr(lastSlash + 1);
                }
                // 限制文件名显示长度
                if (filename.length() > static_cast<size_t>(name_size)) {
                    filename = filename.substr(0, name_size>3?name_size-3:1) + "...";
                }
                ImGui::GetWindowDrawList()->AddText(textPos1, IM_COL32(255, 255, 255, 255), filename.c_str());

                std::string sizeText = std::format("{}x{}", preview.width, preview.height);
                ImGui::GetWindowDrawList()->AddText(textPos2, IM_COL32(255, 255, 255, 255), sizeText.c_str());

                std::string idText = std::format("ID: {}", preview.index);
                ImGui::GetWindowDrawList()->AddText(textPos3, IM_COL32(179, 179, 179, 255), idText.c_str());

                // 右键菜单
                if (ImGui::BeginPopupContextItem("##thumbnail_context")) {
                    ImGui::Text("纹理: %s", filename.c_str());
                    ImGui::Separator();
                    ImGui::Text("完整路径: %s", preview.path.c_str());
                    ImGui::Text("尺寸: %d x %d", preview.width, preview.height);
                    ImGui::Text("索引: %d", preview.index);
                    ImGui::Separator();
                    if (ImGui::Button("删除纹理")) {
                        DeleteTexture(static_cast<int>(i));
                        ImGui::CloseCurrentPopup();
                        ImGui::EndPopup();
                        ImGui::EndGroup();
                        ImGui::PopID();
                        break;
                    }
                    ImGui::EndPopup();
                }

                ImGui::EndGroup();

                // 设置下一个项目的位置
                if ((i + 1) % columnsCount != 0 && i + 1 < m_TexturesBuffer.size()) {
                    ImGui::SameLine(0.0f, cellPadding);
                }

                ImGui::PopID();
            }

            ImGui::EndChild();
        }
    }

    ImGui::End();
}

int TextureGui::LoadTextureFromFile(const std::string& path, TextureType type, bool flipY) {
    auto buffer = ImageUtil::ReadImageRGBA(path, flipY);
    if (!buffer) {
        LOGE("TextureGui::LoadTextureFromFile: Failed to load texture: {}", path);
        return -1;
    }
    std::shared_ptr<Texture> texture;
    switch (type) {
        case TextureType_FLAT: {
            auto flatTexture = std::make_shared<FlatTexture>();
            flatTexture->SetData(buffer);
            texture = flatTexture;
            break;
        }
        case TextureType_SIXFACESCUBE: {
            LOGE("TextureGui::LoadTextureFromFile: SIXFACESCUBE texture loading not implemented yet: {}", path);
            return -1;
        }
        case TextureType_SINGLECUBE: {
            auto flatTexture = std::make_shared<SingleCubeTexture>();
            flatTexture->SetData(buffer);
            texture = flatTexture;
            break;
        }
    }

    TexturePreview preview;
    preview.texture = texture;
    preview.path = path;
    preview.width = buffer->GetWidth();
    preview.height = buffer->GetHeight();
    preview.index = static_cast<int>(m_TexturesBuffer.size());

    CreateGLTexture(preview);

    m_TexturesBuffer.push_back(preview);

    LOGI("TextureGui::LoadTextureFromFile: success-> {} (Index: {}, Size: {}x{})", path, preview.index, preview.width, preview.height);
    return preview.index;
}

void TextureGui::DeleteTexture(int index) {
    if (index < 0 || static_cast<size_t>(index) >= m_TexturesBuffer.size()) {
        LOGE("TextureGui::DeleteTexture: Invalid texture index: {}", index);
        return;
    }

    auto& preview = m_TexturesBuffer[index];
    if (preview.textureID != 0) {
        glDeleteTextures(1, &preview.textureID);
    }
    m_TexturesBuffer.erase(m_TexturesBuffer.begin() + index);

    LOGI("TextureGui::DeleteTexture: success-> index {} deleted", index);
}

void TextureGui::CreateGLTexture(TexturePreview& preview) {
    auto flatTexture = std::dynamic_pointer_cast<FlatTexture>(preview.texture);
    if (!flatTexture) {
        LOGE("TextureGui::CreateGLTexture: Invalid texture type for preview at index {}", preview.index);
        return;
    }

    auto buffer = flatTexture->GetData();
    if (!buffer) {
        LOGE("TextureGui::CreateGLTexture: Texture has no data at index {}", preview.index);
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
}
