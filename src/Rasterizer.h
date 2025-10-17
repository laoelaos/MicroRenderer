//
// Created by laoe on 25-9-6.
//

#ifndef RASTERIZER_H
#define RASTERIZER_H

#include <vector>
#include <mutex>

#include "Scene.h"
#include "TGAImage.h"
#include "Shader.h"

enum RasterizerMode {ZTEST = 0, PHONG = 1, PHONG_WITH_SHADOW = 2};

class Rasterizer {
public:
    static Rasterizer& get();

    void rasterize(Scene& scene, RasterizerMode mode);
    void pass(const Scene& scene, RasterizerMode mode);

    void framebuffer_to_TGA(TGAImage& framebuffer);
    void zBuffer_to_TGA(TGAImage& framebuffer);

    void set_msaa(int MSAA);
private:
    Rasterizer();

    void ZtestPipeline(const Model& model);
    void ZtestFragment(Mesh& mesh);
    void PhongPipeline(const Model& model);
    void PhongFragment(const Material& material, Mesh& mesh);
    void build_buffer();

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

    //lock
    int m_tileRows = 20;
    int m_tileCols = 20;
    std::vector<std::mutex> m_tileLocks = std::vector<std::mutex>(m_tileRows * m_tileCols);

    //helper
    mat4 m_model, m_view, m_projection, m_Viewport;
    mat4 m_MVP, m_MV, m_MVit, m_MVPVi;
};

#endif //RASTERIZER_H
