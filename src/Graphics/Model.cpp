//
// Created by laoe on 25-9-4.
//

#include <fstream>
#include <sstream>

#include "Model.h"
#include "ImageUtils.h"

#include <algorithm>
#include <vector>

#include "Geometry.h"
#include "TextureGui.h"
#include "Logger.h"

// File-scope constant for pi to avoid relying on non-standard M_PI
constexpr double PI = 3.1415926535897932384626433832795;

Mesh::Mesh(const DefaultMesh type) {
    // Small helpers for building meshes
    auto addTri = [this](const vec3& a, const vec3& b, const vec3& c,
                         const vec3& na, const vec3& nb, const vec3& nc,
                         const vec2& ta, const vec2& tb, const vec2& tc) {
        triangles.emplace_back(std::array<vec3,3>{a,b,c}, std::array<vec3,3>{na,nb,nc}, std::array<vec2,3>{ta,tb,tc});
    };
    auto addQuad = [&](const vec3& v0, const vec3& v1, const vec3& v2, const vec3& v3, const vec3& n) {
        // v0-v1-v2-v3 in CCW order for outward normal
        constexpr vec2 uv00{0,0}, uv10{1,0}, uv11{1,1}, uv01{0,1};
        addTri(v0, v1, v2, n, n, n, uv00, uv10, uv11);
        addTri(v0, v2, v3, n, n, n, uv00, uv11, uv01);
    };

    switch (type) {
        case CUBE: {
            // Front (+Z)
            addQuad({-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}, {0,0,1});
            // Back (-Z)
            addQuad({ 1,-1,-1}, {-1,-1,-1}, {-1, 1,-1}, { 1, 1,-1}, {0,0,-1});
            // Left (-X)
            addQuad({-1,-1,-1}, {-1,-1, 1}, {-1, 1, 1}, {-1, 1,-1}, {-1,0,0});
            // Right (+X)
            addQuad({ 1,-1, 1}, { 1,-1,-1}, { 1, 1,-1}, { 1, 1, 1}, {1,0,0});
            // Top (+Y)
            addQuad({-1, 1, 1}, { 1, 1, 1}, { 1, 1,-1}, {-1, 1,-1}, {0,1,0});
            // Bottom (-Y)
            addQuad({-1,-1,-1}, { 1,-1,-1}, { 1,-1, 1}, {-1,-1, 1}, {0,-1,0});
            break;
        }
        case SPHERE: {
            const int sectorCount = 36;   // longitude slices
            const int stackCount  = 18;   // latitude stacks
            const double radius   = 1.0;

            // Precompute grid of positions, normals, and texcoords
            const int cols = sectorCount + 1;
            const int rows = stackCount + 1;
            std::vector<vec3> pos(rows * cols);
            std::vector<vec3> nor(rows * cols);
            std::vector<vec2> uv(rows * cols);

            for (int i = 0; i <= stackCount; ++i) {
                const double stackAngle = PI * 0.5 - i * (PI / stackCount); // +PI/2 .. -PI/2
                const double xy = radius * std::cos(stackAngle);
                const double z  = radius * std::sin(stackAngle);
                const double v  = static_cast<double>(i) / stackCount;      // [0,1]

                for (int j = 0; j <= sectorCount; ++j) {
                    const double sectorAngle = j * (2.0 * PI / sectorCount); // 0..2PI
                    const double x = xy * std::cos(sectorAngle);
                    const double y = xy * std::sin(sectorAngle);
                    const double u = static_cast<double>(j) / sectorCount;   // [0,1]

                    const int k = i * cols + j;
                    const vec3 p{x, y, z};
                    pos[k] = p;
                    nor[k] = normalize(p);
                    uv[k]  = {u, v};
                }
            }

            // Build two triangles per quad on the grid
            for (int i = 0; i < stackCount; ++i) {
                for (int j = 0; j < sectorCount; ++j) {
                    int k1 = i * cols + j;
                    int k2 = k1 + cols;

                    addTri(pos[k1+1], pos[k1], pos[k2+1],
                           nor[k1+1], nor[k1], nor[k2+1],
                           uv[k1+1],  uv[k1],  uv[k2+1]);
                    addTri(pos[k1], pos[k2], pos[k2+1],
                           nor[k1], nor[k2], nor[k2+1],
                           uv[k1],  uv[k2],  uv[k2+1]);
                }
            }
            break;
        }
        case PLANE: {
            // y = 0 plane spanning x,z in [-1,1]
            addQuad({-1,0, 1}, { 1,0, 1}, { 1,0,-1}, {-1,0,-1}, {0,1,0});
            break;
        }
        default: {
            // No geometry
            break;
        }
    }
}

Mesh::Mesh(const std::string &filename) {
    LoadFromFile(filename);
}

void Mesh::LoadFromFile(const std::string &filename) {
    triangles = {};

    std::ifstream file(filename);
    if (!file.is_open()) {
        LOGE("Mesh::LoadFromFile: Could not open file: {}", filename);
        return;
    }

    std::string line, flag;
    double f1, f2, f3;
    int i1, i2, i3;
    char trash;

    std::vector<vec3> vertices, vertices_normal;
    std::vector<vec2> vertices_texture;
    std::vector<int> faces;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        flag = "";
        iss >> flag;

        if (flag == "v") {
            iss >> f1 >> f2 >> f3;
            vertices.emplace_back(f1, f2, f3);
        } else if (flag == "vt") {
            iss >> f1 >> f2;
            vertices_texture.emplace_back(f1, f2);
        } else if (flag == "vn") {
            iss >> f1 >> f2 >> f3;
            vertices_normal.emplace_back(f1, f2, f3);
        } else if (flag == "f") {
            int cnt = 0;
            Triangle tri;
            while (iss >> i1 >> trash >> i2 >> trash >> i3) {
                tri.world_vertices[cnt] = vertices[i1 - 1];
                tri.normals[cnt] = vertices_normal[i3 - 1];
                tri.tex_coords[cnt] = vertices_texture[i2 - 1];
                cnt++;
            }
            if (cnt != 3) {
                LOGE("Mesh::LoadFromFile: Only triangular faces are supported. Face has {} vertices in file: {}", cnt, filename);
                return;
            }
            triangles.push_back(tri);
        }
    }

    LOGI("Mesh::LoadFromFile: Successfully loaded mesh from {} (vertices: {}, triangles: {})",
         filename, vertices.size(), triangles.size());
}

mat4 Mesh::GetTransformMatrix() const {
    double cos_x = cos(rotation.x * PI / 180.0);
    double sin_x = sin(rotation.x * PI / 180.0);
    double cos_y = cos(rotation.y * PI / 180.0);
    double sin_y = sin(rotation.y * PI / 180.0);
    double cos_z = cos(rotation.z * PI / 180.0);
    double sin_z = sin(rotation.z * PI / 180.0);

    // X轴旋转矩阵
    mat4 rot_x = {{
        {1, 0, 0, 0},
        {0, cos_x, -sin_x, 0},
        {0, sin_x, cos_x, 0},
        {0, 0, 0, 1}
    }};

    // Y轴旋转矩阵
    mat4 rot_y = {{
        {cos_y, 0, sin_y, 0},
        {0, 1, 0, 0},
        {-sin_y, 0, cos_y, 0},
        {0, 0, 0, 1}
    }};

    // Z轴旋转矩阵
    mat4 rot_z = {{
        {cos_z, -sin_z, 0, 0},
        {sin_z, cos_z, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    }};

    // 缩放矩阵
    mat4 scale_mat = {{
        {scale.x, 0, 0, 0},
        {0, scale.y, 0, 0},
        {0, 0, scale.z, 0},
        {0, 0, 0, 1}
    }};

    // 平移矩阵
    mat4 trans_mat = identity_matrix<4>();
    trans_mat[0][3] = translation.x;
    trans_mat[1][3] = translation.y;
    trans_mat[2][3] = translation.z;

    // 组合变换: 先缩放，再旋转，最后平移
    mat4 rot_mat = rot_z * rot_y * rot_x;
    return trans_mat * rot_mat * scale_mat;
}

void Mesh::ProcessTransform(const mat4 &MVP, const mat4 &MV, const mat4 &MVit) {
    size_t tri_count = triangles.size();
#pragma omp parallel for default(none) shared(MVP, MV, MVit, tri_count)
    for (size_t i = 0; i < tri_count; ++i) {
        triangles[i].ProcessVertices(MVP, MV, MVit);
    }
}

void Mesh::ProcessClipping() {
    // Vertex structure with all interpolable attributes
    struct ClipVtx {
        vec4 clip;   // clip-space position (x,y,z,w) before perspective divide
        vec3 world;  // view-space position for shading
        vec3 normal; // view-space normal
        vec2 uv;     // texture coordinates
        
        static ClipVtx lerp(const ClipVtx& a, const ClipVtx& b, double t) {
            return {
                a.clip   * (1.0 - t) + b.clip   * t,
                a.world  * (1.0 - t) + b.world  * t,
                a.normal * (1.0 - t) + b.normal * t,
                a.uv     * (1.0 - t) + b.uv     * t
            };
        }
        static double plane_value(const ClipVtx& v, int plane) {
            switch (plane) {
                case 0: return  v.clip.x + v.clip.w;  // left:  -x <= w  => x + w >= 0
                case 1: return  v.clip.w - v.clip.x;  // right:  x <= w  => w - x >= 0
                case 2: return  v.clip.y + v.clip.w;  // bottom: -y <= w => y + w >= 0
                case 3: return  v.clip.w - v.clip.y;  // top:     y <= w => w - y >= 0
                case 4: return  v.clip.z + v.clip.w;  // near:   -z <= w => z + w >= 0
                case 5: return  v.clip.w - v.clip.z;  // far:     z <= w => w - z >= 0
                default: return 0.0;
            }
        }
        static bool inside(const ClipVtx& v, int plane) {
            double d = plane_value(v, plane);
            // If w is negative, the homogeneous sign is flipped; reverse inequality
            //return (v.clip.w >= 0.0) ? (d >= 0.0) : (d <= 0.0);
            return d <= 0;
        }
        static std::vector<ClipVtx> clip_against_plane(const std::vector<ClipVtx>& in, int plane) {
            std::vector<ClipVtx> out;
            if (in.empty()) return out;
            const size_t n = in.size();
            for (size_t i = 0; i < n; ++i) {
                const ClipVtx& curr = in[i];
                const ClipVtx& prev = in[(i + n - 1) % n];
                const bool currIn = inside(curr, plane);
                const bool prevIn = inside(prev, plane);
                const double dc = plane_value(curr, plane);
                const double dp = plane_value(prev, plane);
                if (prevIn && currIn) {
                    out.push_back(curr);
                } else if (prevIn && !currIn) {
                    const double denom = dp - dc;
                    if (std::abs(denom) > 1e-12) {
                        const double t = dp / denom;
                        out.push_back(lerp(prev, curr, t));
                    }
                } else if (!prevIn && currIn) {
                    const double denom = dp - dc;
                    if (std::abs(denom) > 1e-12) {
                        const double t = dp > 0 ? dp / denom : -dp / denom;
                        out.push_back(lerp(prev, curr, t));
                    }
                    out.push_back(curr);
                }
            }
            return out;
        }
    };

    const size_t tri_count = triangles.size();
    std::vector<std::vector<Triangle>> thread_results;

#pragma omp parallel default(none) shared(tri_count, thread_results, triangles)
    {
        std::vector<Triangle> local_tris;
        local_tris.reserve(256);

#pragma omp for schedule(dynamic, 64) nowait
        for (size_t idx = 0; idx < tri_count; ++idx) {
            const Triangle& tri = triangles[idx];
            if (tri.discard) continue;

            std::array<ClipVtx, 3> base{};
            for (int i = 0; i < 3; ++i) {
                base[i] = ClipVtx{tri.clip_vertices[i], tri.world_vertices[i], tri.normals[i], tri.tex_coords[i]};
            }

            bool trivially_out = false;
            bool trivially_in = true;
            for (int p = 0; p < 6; ++p) {
                int in_cnt = 0;
                for (int i = 0; i < 3; ++i) {
                    if (ClipVtx::inside(base[i], p)) in_cnt++;
                }
                if (in_cnt == 0) { trivially_out = true; break; }
                if (in_cnt != 3) trivially_in = false;
            }
            if (trivially_out) {
                continue;
            }

            if (trivially_in) {
                Triangle t;
                t.discard = false;
                for (int k = 0; k < 3; ++k) {
                    t.clip_vertices[k]   = base[k].clip;
                    t.screen_vertices[k] = base[k].clip.to_vec3_point();
                    t.world_vertices[k]  = base[k].world;
                    t.normals[k]         = base[k].normal;
                    t.tex_coords[k]      = base[k].uv;
                }
                local_tris.push_back(t);
                continue;
            }

            std::vector<ClipVtx> poly{base.begin(), base.end()};
            for (int p = 0; p < 6; ++p) {
                poly = ClipVtx::clip_against_plane(poly, p);
                if (poly.empty()) break;
            }
            if (poly.size() < 3) continue;

            const ClipVtx& v0 = poly[0];
            for (size_t i = 1; i + 1 < poly.size(); ++i) {
                const ClipVtx& v1 = poly[i];
                const ClipVtx& v2 = poly[i + 1];
                Triangle t;
                t.discard = false;
                t.clip_vertices[0] = v0.clip;
                t.clip_vertices[1] = v1.clip;
                t.clip_vertices[2] = v2.clip;
                t.screen_vertices[0] = v0.clip.to_vec3_point();
                t.screen_vertices[1] = v1.clip.to_vec3_point();
                t.screen_vertices[2] = v2.clip.to_vec3_point();
                t.world_vertices[0] = v0.world;
                t.world_vertices[1] = v1.world;
                t.world_vertices[2] = v2.world;
                t.normals[0] = v0.normal;
                t.normals[1] = v1.normal;
                t.normals[2] = v2.normal;
                t.tex_coords[0] = v0.uv;
                t.tex_coords[1] = v1.uv;
                t.tex_coords[2] = v2.uv;
                local_tris.push_back(t);
            }
        }

#pragma omp critical
        {
            thread_results.push_back(std::move(local_tris));
        }
    }

    size_t total_size = 0;
    for (const auto& vec : thread_results) total_size += vec.size();
    std::vector<Triangle> out_tris;
    out_tris.reserve(total_size);
    for (auto& vec : thread_results) {
        out_tris.insert(out_tris.end(), std::make_move_iterator(vec.begin()), std::make_move_iterator(vec.end()));
    }
    triangles.swap(out_tris);
}

void Mesh::ProcessViewport(const mat4 &Viewport) {
    size_t tri_count = triangles.size();
#pragma omp parallel for default(none) shared(Viewport, tri_count)
    for (size_t i = 0; i < tri_count; ++i) {
        triangles[i].ProcessViewport(Viewport);
    }
}

void Mesh::ProcessFaceCulling() {
    size_t tri_count = triangles.size();
#pragma omp parallel for default(none) shared(tri_count)
    for (size_t i = 0; i < tri_count; ++i) {
        triangles[i].set_backface();
    }
}

void Triangle::ProcessVertices(const mat4& MVP, const mat4& MV, const mat4& MVit) {
    for (int i = 0; i < 3; i++) {
        clip_vertices[i] = MVP * world_vertices[i].to_vec4(1.0);  // Store clip-space position
        world_vertices[i] = (MV * world_vertices[i].to_vec4(1.0)).to_vec3_point();
        normals[i] = (MVit * normals[i].to_vec4(0.0)).to_vec3_vec();
    }
}

void Triangle::ProcessViewport(const mat4 &Viewport) {
    for (int i = 0; i < 3; i++) {
        // Map from NDC (screen_vertices) to screen coordinates
        screen_vertices[i] = (Viewport * clip_vertices[i]).to_vec3_point();
    }
}

void Triangle::set_backface() {
    discard = discard || ((screen_vertices[1] - screen_vertices[0]) ^ (screen_vertices[2] - screen_vertices[1])).z < 0;
}

bool Triangle::is_invalid() const {
    return (alpha < 0 || gamma < 0 || beta < 0);
}

void Triangle::get_barycentric(double x, double y) {
    double alpha_denominator = - (screen_vertices[0].x - screen_vertices[1].x) * (screen_vertices[2].y - screen_vertices[1].y) +
        (screen_vertices[0].y - screen_vertices[1].y) * (screen_vertices[2].x - screen_vertices[1].x);
    double beta_denominator = - (screen_vertices[1].x - screen_vertices[2].x) * (screen_vertices[0].y - screen_vertices[2].y) +
        (screen_vertices[1].y - screen_vertices[2].y) * (screen_vertices[0].x - screen_vertices[2].x);

    if (std::abs(alpha_denominator) < 1e-8 || std::abs(beta_denominator) < 1e-8) {
        alpha = -1;
        beta = -1;
        gamma = -1;
        return;
    }

    alpha = (- (x - screen_vertices[1].x) * (screen_vertices[2].y - screen_vertices[1].y) + (y - screen_vertices[1].y) * (screen_vertices[2].x - screen_vertices[1].x)) / alpha_denominator;
    beta = (- (x - screen_vertices[2].x) * (screen_vertices[0].y - screen_vertices[2].y) + (y - screen_vertices[2].y) * (screen_vertices[0].x - screen_vertices[2].x)) / beta_denominator;
    gamma = 1. - alpha - beta;
}

void Triangle::get_barycentric_correct(double x, double y) {
    get_barycentric(x, y);
    if (alpha < 0 || beta < 0 || gamma < 0)
        return;
    double z0 = 1.0/world_vertices[0].z;
    double z1 = 1.0/world_vertices[1].z;
    double z2 = 1.0/world_vertices[2].z;
    c_alpha = alpha * z0 / (alpha * z0 + beta * z1 + gamma * z2);
    c_beta = beta * z1 / (alpha * z0 + beta * z1 + gamma * z2);
    c_gamma = gamma * z2 / (alpha * z0 + beta * z1 + gamma * z2);
}

vec3 Triangle::get_interpolated_normal() const {
    return normalize(normals[0] * c_alpha + normals[1] * c_beta + normals[2] * c_gamma);
}

vec3 Triangle::get_interpolated_world_position() const {
    return world_vertices[0] * c_alpha + world_vertices[1] * c_beta + world_vertices[2] * c_gamma;
}

vec2 Triangle::get_interpolated_tex_coords() const {
    return tex_coords[0] * c_alpha + tex_coords[1] * c_beta + tex_coords[2] * c_gamma;
}

double Triangle::get_interpolated_z() const {
    return screen_vertices[0].z * alpha + screen_vertices[1].z * beta + screen_vertices[2].z * gamma;
}

std::tuple<int, int, int, int> Triangle::find_bounding_box_int(int width, int height) const {
    auto [x_min, x_max] = std::minmax({screen_vertices[0].x, screen_vertices[1].x, screen_vertices[2].x});
    auto [y_min, y_max] = std::minmax({screen_vertices[0].y, screen_vertices[1].y, screen_vertices[2].y});
    x_min = std::max(0, static_cast<int>(std::floor(x_min)));
    x_max = std::min(width - 1, static_cast<int>(std::ceil(x_max)));
    y_min = std::max(0, static_cast<int>(std::floor(y_min)));
    y_max = std::min(height - 1, static_cast<int>(std::ceil(y_max)));
    return {x_min, x_max, y_min, y_max};
}

void Material::load_texture(int i) {
    auto tex = std::dynamic_pointer_cast<FlatTexture>(TextureGui::Get().GetTextureByIndex(i));
    if (tex) {
        texture = tex;
        texture_index = i;
    }
}

void Material::load_normal_map(int i) {
    auto nor = std::dynamic_pointer_cast<FlatTexture>(TextureGui::Get().GetTextureByIndex(i));
    if (nor) {
        normal_map = nor;
        normal_map_index = i;
    }
}

Model::Model(DefaultMesh type) {
    mesh = Mesh(type);
}

Model::Model(const std::string& filename) {
    mesh = Mesh(filename);
}
