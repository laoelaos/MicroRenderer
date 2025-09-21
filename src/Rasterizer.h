//
// Created by laoe on 25-9-6.
//

#ifndef RASTERIZER_H
#define RASTERIZER_H
#include <memory>
#include <utility>
#include <vector>
#include <mutex>

#include "Geometry.h"
#include "Graphics.h"
#include "Model.h"
#include "Shader.h"
#include "TGAImage.h"

enum RasterizerMode {ZTEST, PHONG, PHONG_WITH_SHADOW};

class Rasterizer {
public:
    Rasterizer();

    void build_buffer();

    void set_msaa(int MSAA);
    void set_mode(RasterizerMode mode) { this->mode = mode; }

    void rasterize(Scene& scene);

    void framebuffer_to_TGA(TGAImage& framebuffer);
    void zbuffer_to_TGA(TGAImage& framebuffer);
private:
    void pass(const Scene& scene, RasterizerMode mode);

    void Ztest(const Model& obj_model);
    void Phong_Shadow(const Model& model);

    int get_index(int x, int y) const { return x + y * width; }
    int get_tile_lock(int x, int y) const {
        int tile_x = x / (width / tile_cols);
        int tile_y = y / (height / tile_rows);
        return tile_x + tile_y * tile_cols;
    }

    //basic
    int width, height;
    std::vector<double> z_buffer;
    std::vector<vec3> framebuffer;


    //options
    int MSAA = 1;
    RasterizerMode mode;

    //lock
    int tile_rows = 20;
    int tile_cols = 20;
    std::vector<std::mutex> tile_locks = std::vector<std::mutex>(tile_rows * tile_cols);

    //helper
    TGAImage texture, normal_map;
    std::vector<Light> lights;
    mat4 model, view, projection, viewport;
    mat4 mvpv, mv, mvit, mvpvi;
};

#endif //RASTERIZER_H
