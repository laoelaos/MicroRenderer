//
// Created by laoe on 2025/10/18.
//

#ifndef MICRORENDERER_BUFFER_H
#define MICRORENDERER_BUFFER_H

#include <memory>
#include <algorithm>
#include "Geometry.h"

typedef std::array<unsigned char, 4> RGBA;

template<typename T>
class Buffer {
    int m_width, m_height;
    std::unique_ptr<T[]> m_data;
    int m_size;
public:
    Buffer(int width, int height);

    T Get(int x, int y) const;
    T* GetP(int x, int y);
    T GetBilinear(vec2 uv) const;
    T GetNearest(vec2 uv) const;
    void Set(int x, int y, T val);
    void SetAll(T val);
private:
    [[nodiscard]] int GetIndex(int x, int y) const { return x + y * m_width; }
};

class ImageUtil {
public:
    static std::shared_ptr<Buffer<RGBA>> ReadImageRGBA(const std::string &path);
    static void WriteImage(const char *filename, int w, int h, int comp, const void *data, int strideInBytes, bool flipY);
    RGBA EncodeZ(double z); // [-1, 1]
    double DecodeZ(RGBA& color); // [-1, 1]
};

#endif // MICRORENDERER_BUFFER_H
