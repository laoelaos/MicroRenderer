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
#include "Model.h"
#include "Shader.h"
#include "TGAImage.h"

class Rasterizer {
public:
    Rasterizer(int w, int h);

    // Clear all settings, including transformation matrices, loaded triangles, lights, shaders, textures, and framebuffer, z-buffer
    void clear_all();
    void clear_models() { models = {}; }
    void clear_buffer();
    void clear_lights() { lights = {}; }

    void load_model(const Model& model) { models.push_back(model); }
    void load_lights(const std::vector<light>& lights_);
    void load_fragment_shader(std::shared_ptr<Shader> shader) { fragment_shader = std::move(shader); }
    void set_model_matrix(const mat4& m) { model = m; }
    void set_view_matrix(const mat4& m) { view = m; }
    void set_projection_matrix(const mat4& m) { projection = m;}
    void set_options(int MSAA);

    void rasterize();
    void drawonTGA(TGAImage& framebuffer);
private:
    int get_index(int x, int y) const { return x + y * width; }
    int get_tile_lock(int x, int y) const {
        int tile_x = x / (width / tile_cols);
        int tile_y = y / (height / tile_rows);
        return tile_x + tile_y * tile_cols;
    }
    void pre_z();
    void rasterize_model(const Model& model);

    std::vector<Model> models;

    std::vector<light> lights;
    TGAImage texture, normal_map;

    std::shared_ptr<Shader> fragment_shader;
    mat4 model, view, projection, viewport;

    mat4 mvpv, mv, mvit, mvpvi;

    int width, height;
    std::vector<double> z_buffer;
    std::vector<vec3> framebuffer;
    int MSAA = 1;

    int tile_rows = 20;
    int tile_cols = 20;
    std::vector<std::mutex> tile_locks = std::vector<std::mutex>(tile_rows * tile_cols);
};

#endif //RASTERIZER_H
