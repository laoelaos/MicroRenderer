//
// Created by laoe on 25-9-4.
//

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <cassert>
#include <ostream>
#include <cmath>

template<int n> struct vec {
    double data[n] = {0};
    double& operator[](int i) { assert(i>=0&&i<n); return data[i]; }
    double operator[](int i) const { assert(i>=0&&i<n); return data[i]; }
};

template<> struct vec<2> {
    double x = 0, y = 0;
    double& operator[](const int i)       { assert(i>=0 && i<2); return i ? y : x; }
    double  operator[](const int i) const { assert(i>=0 && i<2); return i ? y : x; }
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
};

template<> struct vec<4> {
    double x = 0, y = 0, z = 0, w = 0;
    double& operator[](const int i)       { assert(i>=0 && i<4); return i ? (1==i ? y : (2==i ? z : w)) : x; }
    double  operator[](const int i) const { assert(i>=0 && i<4); return i ? (1==i ? y : (2==i ? z : w)) : x; }
};

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

template<int n> double norm(const vec<n>& v) { 
    return std::sqrt(v * v);
}

template<int n> vec<n> normalize(vec<n>& v) {
    double n = norm(v);
    for (int i=0; i<n; i++) v[i] /= n;
    return v;
}

template<int row, int col> struct mat {
    vec<col> data[row] = {0};

    vec<col>& operator[](const int i)       { assert(i>=0 && i<row); return data[i]; }
    const vec<col>& operator[](const int i) const { assert(i>=0 && i<row); return data[i]; }

    mat operator*(const mat& other) const {
        static_assert(col == other.row, "Matrix multiplication dimension mismatch");
        mat<row, other.col> res;
        for (int i=0; i<row; i++)
            for (int j=0; j<other.col; j++)
                for (int k=0; k<col; k++)
                    res[i][j] += data[i][k] * other[k][j];
        return res;
    }

    mat operator*(const vec<col>& v) const {
        mat<row,1> res;
        for (int i=0; i<row; i++)
            for (int j=0; j<col; j++)
                res[i][0] += data[i][j] * v[j];
        return res;
    }
};

template<int row, int col> std::ostream& operator<<(std::ostream& out, const mat<row, col>& m) {
    for (int i=0; i<row; i++) {
        out << "| ";
        for (int j=0; j<col; j++) out << m[i][j] << " ";
        out << "|\n";
    }
    return out;
};

typedef vec<2> vec2;
typedef vec<3> vec3;
typedef vec<4> vec4;

typedef mat<3,3> mat3;
typedef mat<4,4> mat4;

#endif //GEOMETRY_H