//
// Created by laoe on 2025/10/18.
//

#include "Texture.h"
#include <cmath>
#include <algorithm>

namespace {
    // 返回应用环绕模式后的坐标
    float applyWrap(float coord, WrapMode mode) {
        switch (mode) {
            case Wrap_REPEAT:
                return coord - floor(coord);
            case Wrap_MIRRORED_REPEAT:
                return 1.0f - std::abs(coord - 2.0f * round(coord * 0.5f) - 1.0f);
            case Wrap_CLAMP_TO_EDGE:
                return std::max(0.0f, std::min(1.0f, coord));
            default: // CLAMP_TO_BORDER and others
                return coord;
        }
    }

    // 返回立方体贴图的面索引, uv = 对应的UV坐标
    int selectFace(const vec3& dir, vec2& uv) {
        float absX = std::abs(dir.x);
        float absY = std::abs(dir.y);
        float absZ = std::abs(dir.z);
        int faceIndex;

        if (absX >= absY && absX >= absZ) { // X-face
            faceIndex = dir.x > 0 ? 0 : 1;
            uv.x = (dir.x > 0 ? dir.z / absX + 1.0f : -dir.z / absX + 1.0f) * 0.5f;
            uv.y = (dir.y / absX + 1.0f) * 0.5f;
        } else if (absY >= absX && absY >= absZ) { // Y-face
            faceIndex = dir.y > 0 ? 2 : 3;
            uv.x = (dir.x / absY + 1.0f) * 0.5f;
            uv.y = (dir.y > 0 ? dir.z / absY + 1.0f : -dir.z / absY + 1.0f) * 0.5f;
        } else { // Z-face
            faceIndex = dir.z > 0 ? 4 : 5;
            uv.x = (dir.z > 0 ? -dir.x / absZ + 1.0f : dir.x / absZ + 1.0f) * 0.5f;
            uv.y = (dir.y / absZ + 1.0f) * 0.5f;
        }
        return faceIndex;
    }
}

RGBA FlatTexture::Get(vec2 uv) {
    if (!m_data) return {0, 0, 0, 255};

    const auto& sampler = GetSamplerDesc();

    // Warp mode
    uv.x = applyWrap(uv.x, sampler.wrapS);
    uv.y = applyWrap(uv.y, sampler.wrapT);
    if (sampler.wrapS == Wrap_CLAMP_TO_BORDER || sampler.wrapT == Wrap_CLAMP_TO_BORDER) {
        if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
            return (sampler.borderColor == Border_BLACK) ? RGBA{0, 0, 0, 255} : RGBA{255, 255, 255, 255};
        }
    }

    // Filter mode
    if (sampler.filterMag == Filter_LINEAR) {
        return m_data->GetBilinear(uv);
    }
    return m_data->GetNearest(uv);
}

RGBA SixFacesCubeTexture::Get(vec3 dir) {
    vec2 uv;
    int faceIndex = selectFace(dir, uv);

    if (faceIndex < 0 || faceIndex >= 6 || !m_data[faceIndex]) {
        return {0, 0, 0, 255}; // Return black if face data is missing
    }

    const auto& sampler = GetSamplerDesc();

    // Warp mode
    // Cube maps typically use CLAMP_TO_EDGE, but we can support others
    uv.x = applyWrap(uv.x, sampler.wrapS);
    uv.y = applyWrap(uv.y, sampler.wrapT);

    // Filter mode
    if (sampler.filterMag == Filter_LINEAR) {
        return m_data[faceIndex]->GetBilinear(uv);
    }
    return m_data[faceIndex]->GetNearest(uv);
}

RGBA SingleCubeTexture::Get(vec3 dir) {
    if (!m_data) return {0, 0, 0, 255};

    // uv.x 为水平角度(经度)，uv.y 为垂直角度(纬度)
    vec2 uv;
    const auto& sampler = GetSamplerDesc();

    if (m_type == CubeTexType_Spherical) {
        uv.x = 0.5 + atan2(dir.z, dir.x) / (2 * M_PI);
        uv.y = 0.5 - asin(dir.y) / M_PI;

        // Warp mode
        // spherical maps often use REPEAT for u and CLAMP for v
        uv.x = applyWrap(uv.x, sampler.wrapS);
        uv.y = applyWrap(uv.y, sampler.wrapT);
    } else if (m_type == CubeTexType_Cube) {
        vec2 face_uv;
        int faceIndex = selectFace(dir, face_uv);

        // Map face index to the block coordinate in a 4x3 cross layout
        //     [+Y]
        // [-X][+Z][+X][-Z]
        //     [-Y]

        float bx = 0.f, by = 0.f;
        switch (faceIndex) {
            case 0: bx = 2; by = 1; break; // +X
            case 1: bx = 0; by = 1; break; // -X
            case 2: bx = 1; by = 0; break; // +Y
            case 3: bx = 1; by = 2; break; // -Y
            case 4: bx = 1; by = 1; break; // +Z
            case 5: bx = 3; by = 1; break; // -Z
        }

        // Scale and offset the face UVs to the full texture's UV space
        uv.x = (face_uv.x + bx) / 4.0f;
        uv.y = (face_uv.y + by) / 3.0f;
    }

    // Filter mode
    if (sampler.filterMag == Filter_LINEAR) {
        return m_data->GetBilinear(uv);
    }
    return m_data->GetNearest(uv);
}
