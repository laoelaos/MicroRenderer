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

// void draw_line(int x0, int y0, int x1, int y1, TGAColor color) {
//     if (x0 < 0 || x0 >= width || x1 < 0 || x1 >= width ||
//         y0 < 0 || y0 >= height || y1 < 0 || y1 >= height) {
//         return;
//     }

//     bool steep = false;
//     if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
//         std::swap(x0, y0);
//         std::swap(x1, y1);
//         steep = true;
//     }
//     if (x0 > x1) {
//         std::swap(x0, x1);
//         std::swap(y0, y1);
//     }

//     float y = y0;
//     for (int x = x0; x < x1; x++) {
//         if (steep) {
//             framebuffer.set(y, x, color);
//         } else {
//             framebuffer.set(x, y, color);
//         }
//         y += (y1 - y0) / static_cast<float>(x1 - x0);
//     }
// }

std::tuple<int, int, int> vec3_to_tuple_extend(vec3 vertex, int height, int width) {
    return {(vertex.x + 1.) * width / 2, (vertex.y + 1.) * height / 2, (vertex.z + 1.) * 255 / 2};
}

void draw_triangle(vec3 p0, vec3 p1, vec3 p2, 
    TGAImage& framebuffer, TGAImage& z_buffer, TGAColor color) {

    auto [x0, y0, z0] = vec3_to_tuple_extend(p0, framebuffer.height(), framebuffer.width());
    auto [x1, y1, z1] = vec3_to_tuple_extend(p1, framebuffer.height(), framebuffer.width());
    auto [x2, y2, z2] = vec3_to_tuple_extend(p2, framebuffer.height(), framebuffer.width());

    int x_min = std::min(x0, std::min(x1, x2));
    int x_max = std::max(x0, std::max(x1, x2));
    int y_min = std::min(y0, std::min(y1, y2));
    int y_max = std::max(y0, std::max(y1, y2));
//#pragma omp parallel for
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            double denominator1 = -(x0-x1)*(y2-y1)+(y0-y1)*(x2-x1);
            double denominator2 = -(x1-x2)*(y0-y2)+(y1-y2)*(x0-x2);
            
            // check if the denominators are too small even zero
            if (std::abs(denominator1) < 1e-8 || std::abs(denominator2) < 1e-8) 
                continue;
            
            double alpha = (-(x-x1)*(y2-y1)+(y-y1)*(x2-x1)) / denominator1;
            double beta  = (-(x-x2)*(y0-y2)+(y-y2)*(x0-x2)) / denominator2;
            double gamma = 1.0 - alpha - beta;

            // negative barycentric coordinate => the pixel is outside the triangle
            if (alpha<0 || beta<0 || gamma<0) 
                continue;

            unsigned char z = alpha * z0 + beta * z1 + gamma * z2;
            if (z_buffer.get(x, y)[0] < z) {
                framebuffer.set(x, y, color);
                z_buffer.set(x, y, {z});
            }
        }
    }
}

int main(int argc, char** argv) {
    constexpr int width  = 1000;
    constexpr int height = 1000;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage z_buffer(width, height, TGAImage::GRAYSCALE);

    Model model(argv[1]);

    for (int i=0; i<model.getNumberFace(); i++) { // iterate through all triangles
        vec3 v0t1 = model.getVertex(i, 1) - model.getVertex(i, 0);
        vec3 v1t2 = model.getVertex(i, 2) - model.getVertex(i, 1);
        vec3 n = v0t1 ^ v1t2;
        n = normalize(n);

        TGAColor rnd{static_cast<unsigned char>(n.x * 255), static_cast<unsigned char>(n.y * 255), static_cast<unsigned char>(n.z * 255), 255};
        draw_triangle(model.getVertex(i, 0), model.getVertex(i, 1), model.getVertex(i, 2),
            framebuffer, z_buffer, rnd);
        
    }  

    z_buffer.write_tga_file("z_buffer.tga");
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}

