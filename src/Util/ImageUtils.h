//
// Created by laoe on 2025/10/19.
//

#ifndef MICRORENDERER_IMAGEUTILS_H
#define MICRORENDERER_IMAGEUTILS_H
#include "Buffer.h"


class ImageUtil {
public:
    static std::shared_ptr<Buffer<RGBA>> ReadImageRGBA(const std::string &path, bool flipY);
    static void WriteImage(const char *filename, int w, int h, int comp, const void *data, int strideInBytes, bool flipY);
    static RGBA EncodeZ(double z); // [-1, 1]
    static double DecodeZ(const RGBA& color); // [-1, 1]
    static vec3 RGBAtoVec3(const RGBA& color);
    static RGBA Vec3ToRGBA(const vec3& v);
};


#endif //MICRORENDERER_IMAGEUTILS_H