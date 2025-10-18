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

template<typename T>
Buffer<T>::Buffer(int width, int height) :m_width(width), m_height(height) {
    m_size = width * height;
    m_data = std::make_unique<T[]>(m_size);
}

template<typename T>
inline T Buffer<T>::Get(int x, int y) const {
    // Correctly handle negative coordinates for repeat mode
    int w = m_width;
    int h = m_height;
    return m_data[GetIndex(((x % w) + w) % w, ((y % h) + h) % h)];
}

template<typename T>
inline T* Buffer<T>::GetP(int x, int y) {
    // Correctly handle negative coordinates for repeat mode
    int w = m_width;
    int h = m_height;
    T *ptr = m_data.get();
    return &ptr[GetIndex(((x % w) + w) % w, ((y % h) + h) % h)];
}

template<typename T>
inline T Buffer<T>::GetBilinear(vec2 uv) const {
    float u = uv.x * m_width - 0.5f;
    float v = uv.y * m_height - 0.5f;
    int x = static_cast<int>(floor(u));
    int y = static_cast<int>(floor(v));
    float u_ratio = u - x;
    float v_ratio = v - y;

    RGBA c00 = Get(x, y);
    RGBA c10 = Get(x + 1, y);
    RGBA c01 = Get(x, y + 1);
    RGBA c11 = Get(x + 1, y + 1);

    RGBA result;
    for (int i = 0; i < 4; ++i) {
        float a = c00[i] * (1 - u_ratio) + c10[i] * u_ratio;
        float b = c01[i] * (1 - u_ratio) + c11[i] * u_ratio;
        result[i] = static_cast<unsigned char>(a * (1 - v_ratio) + b * v_ratio);
    }
    return result;
}

template<typename T>
inline T Buffer<T>::GetNearest(vec2 uv) const {
    return Get(static_cast<int>(uv.x * m_width), static_cast<int>(uv.y * m_height));
}

template<typename T>
inline void Buffer<T>::Set(int x, int y, T val) {
    m_data[GetIndex(x, y)] = val;
}

template<typename T>
inline void Buffer<T>::SetAll(T val) {
    std::fill_n(m_data.get(), m_size, val);
}

#endif // MICRORENDERER_BUFFER_H
