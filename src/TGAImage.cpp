#include <iostream>
#include <cstring>
#include "TGAImage.h"

#include <algorithm>

TGAColor::TGAColor(const std::uint8_t R, const std::uint8_t G, const std::uint8_t B, const std::uint8_t A,
                   const std::uint8_t bytespp) {
    bgra[0] = B;
    bgra[1] = G;
    bgra[2] = R;
    bgra[3] = A;
    this->bytespp = bytespp;
}

TGAColor::TGAColor(vec3 color) {
    bgra[0] = static_cast<std::uint8_t>(std::max(0., std::min(255., color.z * 255.)));
    bgra[1] = static_cast<std::uint8_t>(std::max(0., std::min(255., color.y * 255.)));
    bgra[2] = static_cast<std::uint8_t>(std::max(0., std::min(255., color.x * 255.)));
    bgra[3] = 255;
}

#define z_min (-1.0)
#define z_max 1.0
TGAColor::TGAColor(double color) {
    // 将 z 值规范化到 [0, 1] 范围
    double normalized_z = std::clamp((color - z_min) / (z_max - z_min), 0.0, 1.0);

    // 将 [0, 1] 范围的值映射到 32 位整数范围
    uint32_t int_value = static_cast<uint32_t>(normalized_z * 0xFFFFFFFF);

    // 分解为 4 个 8 位通道
    bgra[2] = (int_value >> 24) & 0xFF;  // 最高位字节
    bgra[1] = (int_value >> 16) & 0xFF;  // 次高位字节
    bgra[0] = (int_value >> 8) & 0xFF;   // 次低位字节
    bgra[3] = int_value & 0xFF;          // 最低位字节
}

vec3 TGAColor::to_vec3_rgb() const {
    return {static_cast<double>(bgra[2]) / 255.0,
            static_cast<double>(bgra[1]) / 255.0,
            static_cast<double>(bgra[0]) / 255.0};
}

double TGAColor::to_double() const {
    // 从 4 个 8 位通道重构 32 位整数
    uint32_t int_value = (static_cast<uint32_t>(bgra[2]) << 24) |  // r
                         (static_cast<uint32_t>(bgra[1]) << 16) |  // g
                         (static_cast<uint32_t>(bgra[0]) << 8) |   // b
                         static_cast<uint32_t>(bgra[3]);           // a

    // 转换回 [0, 1] 范围的 double
    double normalized_z = static_cast<double>(int_value) / 0xFFFFFFFF;

    // 转换回原始的 z 值范围 [-1, 1]
    return z_min + normalized_z * (z_max - z_min);
}

vec3 TGAImage::get_nearest(vec2 uv) const {
    return get(static_cast<int>(uv.x * w), static_cast<int>(uv.y * h)).to_vec3_rgb();
}

vec3 TGAImage::get_bilinear(vec2 uv) const {
    int x = static_cast<int>(uv.x * w);
    int y = static_cast<int>(uv.y * h);
    int xp = std::min(x + 1, w - 1);
    int yp = std::min(y + 1, h - 1);
    double u = uv.x * w - x;
    double v = uv.y * h - y;
    vec3 c00 = get(x, y).to_vec3_rgb();
    vec3 c10 = get(xp, y).to_vec3_rgb();
    vec3 c01 = get(x, yp).to_vec3_rgb();
    vec3 c11 = get(xp, yp).to_vec3_rgb();
    return (1 - u) * (1 - v) * c00 +
           u * (1 - v) * c10 +
           (1 - u) * v * c01 +
           u * v * c11;
}

TGAImage::TGAImage(const int w, const int h, const int bpp) : w(w), h(h), bpp(bpp), data(w*h*bpp, 0) {}

bool TGAImage::read_tga_file(const std::string filename) {
    std::ifstream in;
    in.open(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "can't open file " << filename << "\n";
        return false;
    }
    TGAHeader header;
    in.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (!in.good()) {
        std::cerr << "an error occured while reading the header\n";
        return false;
    }
    w   = header.width;
    h   = header.height;
    bpp = header.bitsperpixel>>3;
    if (w<=0 || h<=0 || (bpp!=GRAYSCALE && bpp!=RGB && bpp!=RGBA)) {
        std::cerr << "bad bpp (or width/height) value\n";
        return false;
    }
    size_t nbytes = bpp*w*h;
    data = std::vector<std::uint8_t>(nbytes, 0);
    if (3==header.datatypecode || 2==header.datatypecode) {
        in.read(reinterpret_cast<char *>(data.data()), nbytes);
        if (!in.good()) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
    } else if (10==header.datatypecode||11==header.datatypecode) {
        if (!load_rle_data(in)) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
    } else {
        std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
        return false;
    }
    if (!(header.imagedescriptor & 0x20))
        flip_vertically();
    if (header.imagedescriptor & 0x10)
        flip_horizontally();
    //std::cerr << w << "x" << h << "/" << bpp*8 << "\n";
    return true;
}

bool TGAImage::load_rle_data(std::ifstream &in) {
    size_t pixelcount = w*h;
    size_t currentpixel = 0;
    size_t currentbyte  = 0;
    TGAColor colorbuffer;
    do {
        std::uint8_t chunkheader = 0;
        chunkheader = in.get();
        if (!in.good()) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
        if (chunkheader<128) {
            chunkheader++;
            for (int i=0; i<chunkheader; i++) {
                in.read(reinterpret_cast<char *>(colorbuffer.bgra), bpp);
                if (!in.good()) {
                    std::cerr << "an error occured while reading the header\n";
                    return false;
                }
                for (int t=0; t<bpp; t++)
                    data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel>pixelcount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        } else {
            chunkheader -= 127;
            in.read(reinterpret_cast<char *>(colorbuffer.bgra), bpp);
            if (!in.good()) {
                std::cerr << "an error occured while reading the header\n";
                return false;
            }
            for (int i=0; i<chunkheader; i++) {
                for (int t=0; t<bpp; t++)
                    data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel>pixelcount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        }
    } while (currentpixel < pixelcount);
    return true;
}

bool TGAImage::write_tga_file(const std::string filename, const bool vflip, const bool rle) const {
    constexpr std::uint8_t developer_area_ref[4] = {0, 0, 0, 0};
    constexpr std::uint8_t extension_area_ref[4] = {0, 0, 0, 0};
    constexpr std::uint8_t footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};
    std::ofstream out;
    out.open(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "can't open file " << filename << "\n";
        return false;
    }
    TGAHeader header = {};
    header.bitsperpixel = bpp<<3;
    header.width  = w;
    header.height = h;
    header.datatypecode = (bpp==GRAYSCALE ? (rle?11:3) : (rle?10:2));
    header.imagedescriptor = vflip ? 0x00 : 0x20; // top-left or bottom-left origin
    out.write(reinterpret_cast<const char *>(&header), sizeof(header));
    if (!out.good()) goto err;
    if (!rle) {
        out.write(reinterpret_cast<const char *>(data.data()), w*h*bpp);
        if (!out.good()) goto err;
    } else if (!unload_rle_data(out)) goto err;
    out.write(reinterpret_cast<const char *>(developer_area_ref), sizeof(developer_area_ref));
    if (!out.good()) goto err;
    out.write(reinterpret_cast<const char *>(extension_area_ref), sizeof(extension_area_ref));
    if (!out.good()) goto err;
    out.write(reinterpret_cast<const char *>(footer), sizeof(footer));
    if (!out.good()) goto err;
    return true;
err:
    std::cerr << "can't dump the tga file\n";
    return false;
}

bool TGAImage::unload_rle_data(std::ofstream &out) const {
    const std::uint8_t max_chunk_length = 128;
    size_t npixels = w*h;
    size_t curpix = 0;
    while (curpix<npixels) {
        size_t chunkstart = curpix*bpp;
        size_t curbyte = curpix*bpp;
        std::uint8_t run_length = 1;
        bool raw = true;
        while (curpix+run_length<npixels && run_length<max_chunk_length) {
            bool succ_eq = true;
            for (int t=0; succ_eq && t<bpp; t++)
                succ_eq = (data[curbyte+t]==data[curbyte+t+bpp]);
            curbyte += bpp;
            if (1==run_length)
                raw = !succ_eq;
            if (raw && succ_eq) {
                run_length--;
                break;
            }
            if (!raw && !succ_eq)
                break;
            run_length++;
        }
        curpix += run_length;
        out.put(raw ? run_length-1 : run_length+127);
        if (!out.good()) return false;
        out.write(reinterpret_cast<const char *>(data.data()+chunkstart), (raw?run_length*bpp:bpp));
        if (!out.good()) return false;
    }
    return true;
}

TGAColor TGAImage::get(const int x, const int y) const {
    int c_x = x % w;
    int c_y = y % h;
    if (c_x < 0) c_x += w;
    if (c_y < 0) c_y += h;
    if (!data.size()) return {};
    TGAColor ret = {0, 0, 0, 0, bpp};
    const std::uint8_t *p = data.data()+(c_x+c_y*w)*bpp;
    for (int i=bpp; i--; ret.bgra[i] = p[i]);
    return ret;
}

void TGAImage::set(int x, int y, const TGAColor &c) {
    if (!data.size() || x<0 || y<0 || x>=w || y>=h) return;
    memcpy(data.data()+(x+y*w)*bpp, c.bgra, bpp);
}

void TGAImage::flip_horizontally() {
    for (int i=0; i<w/2; i++)
        for (int j=0; j<h; j++)
            for (int b=0; b<bpp; b++)
                std::swap(data[(i+j*w)*bpp+b], data[(w-1-i+j*w)*bpp+b]);
}

void TGAImage::flip_vertically() {
    for (int i=0; i<w; i++)
        for (int j=0; j<h/2; j++)
            for (int b=0; b<bpp; b++)
                std::swap(data[(i+j*w)*bpp+b], data[(i+(h-1-j)*w)*bpp+b]);
}

int TGAImage::width() const {
    return w;
}

int TGAImage::height() const {
    return h;
}
