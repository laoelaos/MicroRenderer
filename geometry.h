//
// Created by laoe on 25-9-4.
//

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <cassert>
#include <ostream>
#include <cmath>

#include "tgaimage.h"

//vector
template<int n> struct vec {
    double data[n] = {0};
    double& operator[](int i) { assert(i>=0&&i<n); return data[i]; }
    double operator[](int i) const { assert(i>=0&&i<n); return data[i]; }
};

template<> struct vec<2> {
    double x = 0, y = 0;
    double& operator[](const int i)       { assert(i>=0 && i<2); return i ? y : x; }
    double  operator[](const int i) const { assert(i>=0 && i<2); return i ? y : x; }
    [[nodiscard]] vec<3> to_vec3() const;
};

template<> struct vec<3> {
    double x = 0, y = 0, z = 0;
    double& operator[](const int i)       { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }
    double  operator[](const int i) const { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }
    vec     operator^(const vec& other) const {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }

    [[nodiscard]] TGAColor to_color() const {
        return {static_cast<unsigned char>(std::max(0., std::min(255., x))),
            static_cast<unsigned char>(std::max(0., std::min(255., y))),
            static_cast<unsigned char>(std::max(0., std::min(255., z))),
            255};
    }

    [[nodiscard]] vec<2> to_vec2() const;
    [[nodiscard]] vec<4> to_vec4(double w_=1.) const;
};

template<> struct vec<4> {
    double x = 0, y = 0, z = 0, w = 0;
    double& operator[](const int i)       { assert(i>=0 && i<4); return i ? (1==i ? y : (2==i ? z : w)) : x; }
    double  operator[](const int i) const { assert(i>=0 && i<4); return i ? (1==i ? y : (2==i ? z : w)) : x; }
    [[nodiscard]] vec<3> to_vec3() const;
};

inline vec<3> vec<2>::to_vec3() const { return {x, y, 0}; }
inline vec<2> vec<3>::to_vec2() const { return {x, y}; }
inline vec<4> vec<3>::to_vec4(double w_) const { return {x, y, z, w_}; }
inline vec<3> vec<4>::to_vec3() const { assert(w != 0); return {x/w, y/w, z/w}; }


template<int n> std::ostream& operator<<(std::ostream& out, const vec<n>& v) {
    out << "(";
    for (int i=0; i<n; i++) out << v[i] << " ";
    out << ")";
    return out;
}

template<int n> vec<n> operator+(const vec<n>& v1, const vec<n>& v2) {
    vec<n> res;
    for (int i=0; i<n; i++) res[i] = v1[i] + v2[i];
    return res;
}

template<int n> vec<n> operator-(const vec<n>& v1, const vec<n>& v2) {
    vec<n> res;
    for (int i=0; i<n; i++) res[i] = v1[i] - v2[i];
    return res;
}

template<int n> double operator*(const vec<n>& v1, const vec<n>& v2) {
    double res = 0;
    for (int i=0; i<n; i++) res += v1[i] * v2[i];
    return res;
}

template<int n> vec<n> operator*(const vec<n>& v, const double scale) {
    vec<n> res;
    for (int i=0; i<n; i++) res[i] = v[i] * scale;
    return res;
}

template<int n> vec<n> operator*(const double scale, const vec<n>& v) {
    vec<n> res;
    for (int i=0; i<n; i++) res[i] = v[i] * scale;
    return res;
}

template<int n> vec<n> operator/(const vec<n>& v, const double scale) {
    vec<n> res;
    for (int i=0; i<n; i++) res[i] = v[i] / scale;
    return res;
}

template<int n> double norm(const vec<n>& v) {
    return std::sqrt(v * v);
}

template<int n> vec<n> normalize_self(vec<n>& v) {
    double c = norm(v);
    v = v / c;
    return v;
}

template<int n> vec<n> normalize(const vec<n> v) {
    double c = norm(v);;
    return v / c;
}

//store horizontal vector
template<int n_row, int n_col> struct mat {
    vec<n_col> data[n_row] = {0};

          vec<n_col>& operator[](const int i)       { assert(i>=0 && i<n_row); return data[i]; }
    const vec<n_col>& operator[](const int i) const { assert(i>=0 && i<n_row); return data[i]; }

    mat transpose() const {
        mat<n_col, n_row> res;
        for (int i=0; i<n_row; i++) {
            for (int j=0; j<n_col; j++) {
                res[j][i] = data[i][j];
            }
        }
        return res;
    }

    [[nodiscard]] double cofactor(int r, int c) const {
        mat<n_row-1, n_col-1> res;
        for (int i=0; i<n_row; i++) {
            if (i == r) continue;
            for (int j=0; j<n_col; j++) {
                if (j == c) continue;
                res[i < r ? i : i-1][j < c ? j : j-1] = data[i][j];
            }
        }
        return ((r+c)&1 ? -1 : 1) * determinant(res);
    }

    mat invert() const {
        mat res;
        double det = determinant(*this);
        assert(std::abs(det) > 1e-8);
        for (int i=0; i<n_row; i++) {
            for (int j=0; j<n_col; j++) {
                res[j][i] = cofactor(i, j) / det;
            }
        }
        return res;
    }
};

template<int n_row, int n_col> std::ostream& operator<<(std::ostream& out, const mat<n_row, n_col>& m) {
    for (int i=0; i<n_row; i++) out << m[i];
    return out;
}

template<int n_row, int n_col> mat<n_row, n_col> operator+(const mat<n_row, n_col>& m1, const mat<n_row, n_col>& m2) {
    mat<n_row, n_col> res;
    for (int i=0; i<n_row; i++) {
        for (int j=0; j<n_col; j++) res[i][j] = m1[i][j] + m2[i][j];
    }
    return res;
}

template<int n_row, int n_col> mat<n_row, n_col> operator-(const mat<n_row, n_col>& m1, const mat<n_row, n_col>& m2) {
    mat<n_row, n_col> res;
    for (int i=0; i<n_row; i++) {
        for (int j=0; j<n_col; j++) res[i][j] = m1[i][j] - m2[i][j];
    }
    return res;
}

template<int n_row, int n_col> mat<n_row, n_col> operator*(const mat<n_row, n_col>& m, const double scale) {
    mat<n_row, n_col> res;
    for (int i=0; i<n_row; i++) {
        for (int j=0; j<n_col; j++) res[i][j] = m[i][j] * scale;
    }
    return res;
}

template<int n_row, int n_col> vec<n_row> operator*(const mat<n_row, n_col>& m, const vec<n_col>& v) {
    vec<n_row> res;
    for (int i=0; i<n_row; i++) {
        res[i] = m[i] * v;
    }
    return res;
}

template<int nr1, int nc1, int nc2> mat<nr1, nc2> operator*(const mat<nr1, nc1>& m1, const mat<nc1, nc2>& m2) {
    mat<nr1, nc2> res;
    for (int i=0; i<nr1; i++) {
        for (int j=0; j<nc2; j++) {
            res[i][j] = 0;
            for (int k=0; k<nc1; k++) res[i][j] += m1[i][k] * m2[k][j];
        }
    }
    return res;
}

template<int n_row, int n_col> mat<n_row, n_col> operator/(const mat<n_row, n_col>& m, const double scale) {
    mat<n_row, n_col> res;
    for (int i=0; i<n_row; i++) {
        for (int j=0; j<n_col; j++) res[i][j] = m[i][j] / scale;
    }
    return res;
}

//not understand
template<int n> double determinant(const mat<n,n>& m) {
    double res = 1;
    mat<n,n> temp = m;
    for (int i=0; i<n; i++) {
        int pivot = i;
        for (int j=i+1; j<n; j++) {
            if (std::abs(temp[j][i]) > std::abs(temp[pivot][i])) pivot = j;
        }
        if (std::abs(temp[pivot][i]) < 1e-8) return 0;
        if (i != pivot) {
            std::swap(temp[i], temp[pivot]);
            res = -res;
        }
        res *= temp[i][i];
        for (int j=i+1; j<n; j++) {
            double factor = temp[j][i] / temp[i][i];
            for (int k=i; k<n; k++) {
                temp[j][k] -= factor * temp[i][k];
            }
        }
    }
    return res;
}

template<int n> mat<n, n> identity_matrix() {
    mat<n,n> res;
    for (int i=0; i<n; i++) res[i][i] = 1;
    return res;
}

//2 double vector, usually be vertical vector
typedef vec<2> vec2;
//3 double vector, usually be vertical vector
typedef vec<3> vec3;
//4 double vector, usually be vertical vector
typedef vec<4> vec4;

//3x3 Matrix, 9 double, store horizontal vector
typedef mat<3,3> mat3;
//4x4 Matrix, 16 double, store horizontal vector
typedef mat<4,4> mat4;

#endif //GEOMETRY_H