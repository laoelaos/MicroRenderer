//
// Created by laoe on 2025/10/19.
//

#ifndef MICRORENDERER_TEXTUREGUI_H
#define MICRORENDERER_TEXTUREGUI_H

#include <vector>

#include "Texture.h"

struct TexturePreview {
    std::shared_ptr<Texture> texture;
    std::string path;

    uint32_t textureID = 0;
    int index = -1;

    int width = 0;
    int height = 0;
};

class TextureGui {
    std::string m_ReadyToLoadPath;
    std::vector<TexturePreview> m_TexturesBuffer;

    TextureGui() = default;
public:
    static TextureGui& Get();

    std::shared_ptr<Texture> GetTextureByIndex(int index);

    void LaunchTextureGui();
    int LoadTextureFromFile(const std::string& path, TextureType type, bool flipY); //返回纹理索引
private:
    void DeleteTexture(int index);
    void CreateGLTexture(TexturePreview& preview);
};


#endif //MICRORENDERER_TEXTUREGUI_H