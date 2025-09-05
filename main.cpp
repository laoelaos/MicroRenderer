#include <atomic>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

constexpr int width  = 1000;
constexpr int height = 1000;
TGAImage framebuffer(width, height, TGAImage::RGB);

void draw_line(int x0, int y0, int x1, int y1, TGAColor color) {
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

void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, TGAColor color) {
    int x_min = std::min(x0, std::min(x1, x2));
    int x_max = std::max(x0, std::max(x1, x2));
    int y_min = std::min(y0, std::min(y1, y2));
    int y_max = std::max(y0, std::max(y1, y2));
//#pragma omp parallel for
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            double denominator1 = -(x0-x1)*(y2-y1)+(y0-y1)*(x2-x1);
            double denominator2 = -(x1-x2)*(y0-y2)+(y1-y2)*(x0-x2);
            
            // 检查分母是否为零
            if (std::abs(denominator1) < 1e-8 || std::abs(denominator2) < 1e-8) 
                continue;
            
            double alpha = (-(x-x1)*(y2-y1)+(y-y1)*(x2-x1)) / denominator1;
            double beta  = (-(x-x2)*(y0-y2)+(y-y2)*(x0-x2)) / denominator2;
            double gamma = 1.0 - alpha - beta;

            // negative barycentric coordinate => the pixel is outside the triangle
            if (alpha<0 || beta<0 || gamma<0) continue;
            framebuffer.set(x, y, color);
        }
    }
}

std::tuple<int, int> vec3_to_tuple_extend(vec3 vertex) {
    return {(vertex.x + 1.) * width / 2, (vertex.y + 1.) * height / 2};
}

int main(int argc, char** argv) {
    // int x0 = 100, y0 = 100;
    // int x1 = 300, y1 = 150;
    // int x2 = 800, y2 = 800;

    // std::srand(std::time({}));
    // for (int i=0; i<(1<<12); i++) {
    //     int ax = rand()%width, ay = rand()%height;
    //     int bx = rand()%width, by = rand()%height;
    //     int cx = rand()%width, cy = rand()%height;
    //     draw_triangle(ax, ay, bx, by, cx, cy, 
    //         {static_cast<unsigned char>(rand()%255), static_cast<unsigned char>(rand()%255),
    //          static_cast<unsigned char>(rand()%255), static_cast<unsigned char>(rand()%255)});
    // }
    //draw_triangle(x0, y0, x1, y1, x2, y2, green);

    Model model(argv[1]);

    for (int i=0; i<model.getNumberFace(); i++) { // iterate through all triangles
        auto [ax, ay] = vec3_to_tuple_extend(model.getVertex(i, 0));
        auto [bx, by] = vec3_to_tuple_extend(model.getVertex(i, 1));
        auto [cx, cy] = vec3_to_tuple_extend(model.getVertex(i, 2));
        vec3 v0t1 = model.getVertex(i, 1) - model.getVertex(i, 0);
        vec3 v1t2 = model.getVertex(i, 2) - model.getVertex(i, 1);
        vec3 n = v0t1 ^ v1t2;
        n = normalize(n);

        if (n.z > 0) {
            TGAColor rnd{static_cast<unsigned char>(n.x * 255), static_cast<unsigned char>(n.y * 255), static_cast<unsigned char>(n.z * 255), 255};
            draw_triangle(ax, ay, bx, by, cx, cy, rnd);
        }
    }    

    return framebuffer.write_tga_file("framebuffer.tga");
}

