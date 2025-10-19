//
// Created by laoe on 2025/10/19.
//

#include "ImageUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <algorithm>

#include "Logger.h"
#include "stb_image.h"
#include "stb_image_write.h"

std::shared_ptr<Buffer<RGBA>> ImageUtil::ReadImageRGBA(const std::string &path, bool flipY) {
    stbi_set_flip_vertically_on_load(flipY);
    int iw = 0, ih = 0, n = 0;
    unsigned char *data = stbi_load(path.c_str(), &iw, &ih, &n, STBI_default);
    if (data == nullptr) {
        LOGE("ImageUtil::ReadImageRGBA: {} read failed", path);
        return nullptr;
    }

    auto buffer = std::make_shared<Buffer<RGBA>>(iw, ih);
    for (int y = 0; y < ih; y++) {
        for (int x = 0; x < iw; x++) {
            auto& to = *buffer->GetP(x, y);
            size_t idx = x + y * iw;

            switch (n) {
                case STBI_grey: {
                    to[0] = data[idx];
                    to[1] = to[2] = to[0];
                    to[3] = 255;
                    break;
                }
                case STBI_grey_alpha: {
                    to[0] = data[idx * 2 + 0];
                    to[1] = to[2] = to[0];
                    to[3] = data[idx * 2 + 1];
                    break;
                }
                case STBI_rgb: {
                    to[0] = data[idx * 3 + 0];
                    to[1] = data[idx * 3 + 1];
                    to[2] = data[idx * 3 + 2];
                    to[3] = 255;
                    break;
                }
                case STBI_rgb_alpha: {
                    to[0] = data[idx * 4 + 0];
                    to[1] = data[idx * 4 + 1];
                    to[2] = data[idx * 4 + 2];
                    to[3] = data[idx * 4 + 3];
                    break;
                }
                default:
                    LOGW("ImageUtil::ReadImageRGBA: {} unknown format n={}", path, n);
                    break;
            }
        }
    }
    stbi_image_free(data);
    return buffer;
}

void ImageUtil::WriteImage(const char *filename, int w, int h, int comp, const void *data, int strideInBytes,
    bool flipY) {
    stbi_flip_vertically_on_write(flipY);
    stbi_write_png(filename, w, h, comp, data, strideInBytes);
}

RGBA ImageUtil::EncodeZ(double z) {
    // clamp to [-1, 1], map to [0, 1]
    double zn = std::max(-1.0, std::min(1.0, z));
    double u = (zn + 1.0) * 0.5; // [0,1]
    // pack into 24 bits RGB
    unsigned int value = static_cast<unsigned int>(u * ((1u << 24) - 1));
    RGBA out{};
    out[0] = static_cast<unsigned char>((value >> 16) & 0xFF);
    out[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
    out[2] = static_cast<unsigned char>(value & 0xFF);
    out[3] = 255;
    return out;
}

double ImageUtil::DecodeZ(const RGBA &color) {
    unsigned int value = (static_cast<unsigned int>(color[0]) << 16) |
                         (static_cast<unsigned int>(color[1]) << 8) |
                          static_cast<unsigned int>(color[2]);
    double u = static_cast<double>(value) / static_cast<double>((1u << 24) - 1);
    double z = u * 2.0 - 1.0;
    return z;
}

vec3 ImageUtil::RGBAtoVec3(const RGBA &color) {
    return vec3(color[0], color[1], color[2]) / 255.0f;
}

RGBA ImageUtil::Vec3ToRGBA(const vec3 &v) {
    return {
        static_cast<unsigned char>(std::clamp(v.x * 255, 0.0, 255.0)),
        static_cast<unsigned char>(std::clamp(v.y * 255, 0.0, 255.0)),
        static_cast<unsigned char>(std::clamp(v.z * 255, 0.0, 255.0)),
        255
    };
}
