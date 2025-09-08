//
// Created by laoe on 25-9-6.
//

#ifndef RASTERIZER_H
#define RASTERIZER_H
#include <memory>
#include <vector>

#include "Geometry.h"
#include "Material.h"
#include "Model.h"
#include "Shader.h"
#include "TGAImage.h"

class Rasterizer {
public:
    Rasterizer(int w, int h);

    // Clear all settings, including transformation matrices, loaded triangles, lights, shaders, textures, and framebuffer, z-buffer
    void clear_all();
    // Clear only the triangles
    void clear_triangles() { triangles = {}; }

    void load_model(const Model& model) { models.push_back(model); }

    void load_triangles(const std::vector<triangle>& triangles_);
    void load_lights(const std::vector<light>& lights_);
    void load_fragment_shader(std::shared_ptr<Shader> shader) { fragment_shader = shader; }

    void set_texture(const TGAImage& tex) { texture = tex; }
    void set_normal_map(const TGAImage& norm) { normal_map = norm; }

    // set options for shading
    void set_options(bool smooth_shading, bool normal_mapping) {
        this->smooth_shading = smooth_shading;
        this->normal_mapping = normal_mapping;
    }

    void set_model_matrix(const mat4& m) { model = m; }
    void set_view_matrix(const mat4& m) { view = m; }
    void set_projection_matrix(const mat4& m) { projection = m;}

    // Perform the rasterization process on the loaded triangles
    void rasterize();
    // Output the framebuffer to a TGAImage
    void drawonTGA(TGAImage& framebuffer);
private:
    [[nodiscard]] int get_index(int x, int y) const { return x + y * width; }

    void rasterize_model(const Model& model);

    std::vector<Model> models;

    std::vector<triangle> triangles;

    std::vector<light> lights;
    TGAImage texture, normal_map;

    std::shared_ptr<Shader> fragment_shader;
    mat4 model, view, projection, viewport;

    mat4 mvpv, mv, mvit, mvpvi;

    int width, height;
    std::vector<double> z_buffer;
    std::vector<vec3> framebuffer;

    bool smooth_shading = true;
    bool normal_mapping = false;
};

#endif //RASTERIZER_H
