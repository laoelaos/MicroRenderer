#include <atomic>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>

#include "tgaimage.h"
#include "model.h"

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

constexpr int width  = 1000;
constexpr int height = 1000;
TGAImage framebuffer(width, height, TGAImage::RGB);

void draw_line(std::tuple<int, int> verx, std::tuple<int, int> very, TGAColor color) {
    auto [x0, y0] = verx;
    auto [x1, y1] = very;
    if (x0 < 0 || x0 >= width || x1 < 0 || x1 >= width ||
        y0 < 0 || y0 >= height || y1 < 0 || y1 >= height) {
        return;
    }

    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    float y = y0;
    for (int x = x0; x < x1; x++) {
        if (steep) {
            framebuffer.set(y, x, color);
        } else {
            framebuffer.set(x, y, color);
        }
        y += (y1 - y0) / static_cast<float>(x1 - x0);
    }
}

std::tuple<int, int> vec3_to_tuple_extend(vec3 vertex) {
    return {(vertex.x + 1.) * width / 2, (vertex.y + 1.) * height / 2};
}

int main(int argc, char** argv) {
    Model tm(argv[1]);
    std::cout << "no of f:" << tm.getNumberFace();
    for (int i = 0; i < tm.getNumberFace(); i++) {
        draw_line(vec3_to_tuple_extend(tm.getVertex(i, 0)),
            vec3_to_tuple_extend(tm.getVertex(i, 1)), red);
        draw_line(vec3_to_tuple_extend(tm.getVertex(i, 2)),
            vec3_to_tuple_extend(tm.getVertex(i, 1)), red);
        draw_line(vec3_to_tuple_extend(tm.getVertex(i, 0)),
            vec3_to_tuple_extend(tm.getVertex(i, 2)), red);
    }


    return framebuffer.write_tga_file("framebuffer.tga");
}

