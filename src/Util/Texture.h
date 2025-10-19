//
// Created by laoe on 2025/10/18.
//

#ifndef MICRORENDERER_TEXTURE_H
#define MICRORENDERER_TEXTURE_H
#include <memory>

#include "Geometry.h"
#include "Buffer.h"

enum WrapMode {
    Wrap_REPEAT = 0,      // 重复模式
    Wrap_MIRRORED_REPEAT, // 镜像重复模式
    Wrap_CLAMP_TO_EDGE,   // 边缘拉伸模式
    Wrap_CLAMP_TO_BORDER, // 边界颜色模式
};
enum FilterMode {
    Filter_NEAREST = 0,            // 最近点采样
    Filter_LINEAR,                 // 线性插值
    Filter_NEAREST_MIPMAP_NEAREST, // 最近点采样（最近mipmap级别）
    Filter_LINEAR_MIPMAP_NEAREST,  // 线性插值（最近mipmap级别）
    Filter_NEAREST_MIPMAP_LINEAR,  // 最近点采样（线性mipmap级别）
    Filter_LINEAR_MIPMAP_LINEAR,   // 线性插值（线性mipmap级别）
};
enum BorderColor {
    Border_BLACK = 0, // 黑色边界
    Border_WHITE,     // 白色边界
};
struct SamplerDesc {
    FilterMode filterMin = Filter_NEAREST; // 缩小过滤模式
    FilterMode filterMag = Filter_NEAREST; // 放大过滤模式

    WrapMode wrapS = Wrap_CLAMP_TO_EDGE;   // S坐标环绕模式
    WrapMode wrapT = Wrap_CLAMP_TO_EDGE;   // T坐标环绕模式
    WrapMode wrapR = Wrap_CLAMP_TO_EDGE;   // R坐标环绕模式（3D纹理）
    BorderColor borderColor = Border_BLACK; // 边界颜色
};

enum TextureType {
    TextureType_Flat = 0,
    TextureType_SixFacesCube,
    TextureType_SingleCube,
};
class Texture {
    SamplerDesc m_SamplerDesc;
public:
    virtual ~Texture() = default;
    virtual TextureType GetType() = 0;

    const SamplerDesc& GetSamplerDesc() const { return m_SamplerDesc; }
    void SetSamplerDesc(const SamplerDesc &sampler) { m_SamplerDesc = sampler; }
};

class FlatTexture : public Texture {
    std::shared_ptr<Buffer<RGBA>> m_data;
public:
    TextureType GetType() override { return TextureType_Flat; }
    RGBA Get(vec2 uv);
    void SetData(const std::shared_ptr<Buffer<RGBA>> &data) { m_data = data; }
    [[nodiscard]] std::shared_ptr<Buffer<RGBA>> GetData() const { return m_data; }
};

class SixFacesCubeTexture : public Texture {
    std::array<std::shared_ptr<Buffer<RGBA>>, 6> m_data;
public:
    TextureType GetType() override { return TextureType_SixFacesCube; }
    RGBA Get(vec3 dir);
    void SetData(int i, const std::shared_ptr<Buffer<RGBA>> &data) { m_data[i] = data; }
    [[nodiscard]] std::shared_ptr<Buffer<RGBA>> GetData(int i) const { return m_data[i]; }
};

enum CubeTexType {
    CubeTexType_Spherical = 0,
    CubeTexType_Cube,
};
class SingleCubeTexture : public Texture {
    std::shared_ptr<Buffer<RGBA>> m_data;
    CubeTexType m_type = CubeTexType_Spherical;
public:
    TextureType GetType() override { return TextureType_SingleCube; }
    RGBA Get(vec3 dir);
    void SetType(CubeTexType type) { m_type = type; }
    void SetData(const std::shared_ptr<Buffer<RGBA>> &data) { m_data = data; }
    [[nodiscard]] std::shared_ptr<Buffer<RGBA>> GetData() const { return m_data; }
};

#endif //MICRORENDERER_TEXTURE_H
