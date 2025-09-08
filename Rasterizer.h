//
// Created by laoe on 25-9-6.
//

#ifndef RASTERIZER_H
#define RASTERIZER_H
#include <memory>
#include <vector>

#include "geometry.h"
#include "Shader.h"
#include "tgaimage.h"

class Rasterizer {
public:
    Rasterizer(int w, int h);
    void clear_all();
    void clear_triangles() { triangles = {}; }

    void load_triangles(const std::vector<triangle>& triangles_);
    void load_lights(const std::vector<light>& lights_);
    void load_fragment_shader(std::shared_ptr<Shader> shader) { fragment_shader = shader; }

    void set_texture(const TGAImage& tex) { texture = tex; }
    void set_normal_map(const TGAImage& norm) { normal_map = norm; }

    void set_options(bool smooth_shading, bool normal_mapping) {
        this->smooth_shading = smooth_shading;
        this->normal_mapping = normal_mapping;
    }

    void set_model_matrix(const mat4& m) { model = m; }
    void set_view_matrix(const mat4& m) { view = m; }
    void set_projection_matrix(const mat4& m) { projection = m;}

    void rasterize();
    void drawonTGA(TGAImage& framebuffer);
private:
    [[nodiscard]] int get_index(int x, int y) const { return x + y * width; }
    void rasterize_triangle(triangle triangle_, vec3 world_pos[]);

    std::vector<triangle> triangles;

    std::vector<light> lights;
    TGAImage texture, normal_map;

    std::shared_ptr<Shader> fragment_shader;
    mat4 model, view, projection, viewport, mvpv, mv;

    int width, height;
    std::vector<double> z_buffer;
    std::vector<vec3> framebuffer;

    bool smooth_shading = true;
    bool normal_mapping = false;
};

#endif //RASTERIZER_H
