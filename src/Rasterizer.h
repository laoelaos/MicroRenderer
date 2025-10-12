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
#include "TGAImage.h"

enum RasterizerMode {ZTEST, PHONG, PHONG_WITH_SHADOW};

class Rasterizer {
public:
    Rasterizer();

    void build_buffer();

    void set_msaa(int MSAA);
    void set_mode(RasterizerMode mode) { this->m_mode = mode; }

    void rasterize(Scene& scene);

    void framebuffer_to_TGA(TGAImage& framebuffer);
    void zbuffer_to_TGA(TGAImage& framebuffer);
private:
    void pass(const Scene& scene, RasterizerMode mode);

    void Ztest(const Model& model);
    void Phong(const Model& model);

    int get_index(int x, int y) const { return x + y * m_width; }
    int get_tile_lock(int x, int y) const {
        int tile_x = x / (m_width / m_tileCols);
        int tile_y = y / (m_height / m_tileRows);
        return tile_x + tile_y * m_tileCols;
    }

    //basic
    int m_width, m_height;
    std::vector<double> m_zBuffer;
    std::vector<vec3> m_frameBuffer;

    //options
    int m_MSAA = 1;
    RasterizerMode m_mode;

    //lock
    int m_tileRows = 20;
    int m_tileCols = 20;
    std::vector<std::mutex> m_tileLocks = std::vector<std::mutex>(m_tileRows * m_tileCols);

    //helper
    TGAImage m_texture, m_normalMap;
    std::vector<Light> m_lights;
    mat4 m_model, m_view, m_projection, m_viewport;
    mat4 m_MVPV, m_MV, m_MVit, m_MVPVi;
};

#endif //RASTERIZER_H
