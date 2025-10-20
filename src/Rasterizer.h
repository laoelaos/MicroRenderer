//
// Created by laoe on 25-9-6.
//

#ifndef RASTERIZER_H
#define RASTERIZER_H

#include <vector>
#include <mutex>
#include <memory>

#include "Scene.h"
#include "Buffer.h"
#include "Shader.h"

enum RasterizerMode {
    RasterizerMode_ZTEST = 0,
    RasterizerMode_PHONG = 1,
    RasterizerMode_PHONG_SHADOW = 2,
    RasterizerMode_SKYBOX = 3
};

class Rasterizer {
public:
    static Rasterizer& get();

    void pass(const Scene& scene, RasterizerMode mode, FrameBuffer& frame_buffer);

    void set_msaa(int MSAA);
private:
    Rasterizer();

    void ZtestPipeline(const Model& model);
    void ZtestFragment(Mesh& mesh);
    void PhongPipeline(const Model& model);
    void PhongFragment(const Material& material, Mesh& mesh);
    void SkyboxPipeline(const Scene& scene);
    void SkyboxFragment(const std::shared_ptr<SingleCubeTexture>& skybox_texture);
    void FillInColor();
    void FillInZVal();

    void build_buffer();

    int get_tile_lock(int x, int y) const {
        int tile_x = x / (m_width / m_tileCols);
        int tile_y = y / (m_height / m_tileRows);
        return tile_x + tile_y * m_tileCols;
    }

    //basic
    int m_width, m_height;
    Buffer<double> m_zBuffer;
    Buffer<vec3> m_colorBuffer;
    std::shared_ptr<Buffer<RGBA>> m_finalZBuffer;
    std::shared_ptr<Buffer<RGBA>> m_finalColorBuffer;

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
