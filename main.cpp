#include <cmath>
#include <iostream>

#include "tgaimage.h"

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

constexpr int width  = 64;
constexpr int height = 64;
TGAImage framebuffer(width, height, TGAImage::RGB);

void draw_line(int x0, int y0, int x1, int y1, TGAColor color) {
    constexpr int step = 50;
    const double x_pre = static_cast<double>(x1 - x0) / step, y_pre = static_cast<double>(y1 - y0) / step;
    double x_now = x0, y_now = y0;
    for (int i = 0; i < step; i++) {
        x_now += x_pre;
        y_now += y_pre;
        framebuffer.set(static_cast<int>(std::round(x_now)), static_cast<int>(std::round(y_now)), color);
    }
}

int main(int argc, char** argv) {
    int ax =  7, ay =  3;
    int bx = 12, by = 37;
    int cx = 62, cy = 53;

    draw_line(ax, ay, bx, by, red);
    draw_line(bx, by, cx, cy, green);
    draw_line(ax, ay, cx, cy, yellow);

    framebuffer.set(ax, ay, white);
    framebuffer.set(bx, by, white);
    framebuffer.set(cx, cy, white);

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}

