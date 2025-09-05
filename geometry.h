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
    double  operator[](int i) const { assert(i>=0&&i<n); return data[i]; }
};

template<int n> std::ostream& operator<<(std::ostream& out, const vec<n>& v) {
    out << "(";
    for (int i=0; i<n; i++) out << v[i] << " ";
    out << ")";
    return out;
}

template<> struct vec<3> {
    double x = 0, y = 0, z = 0;

    double  norm() const { return std::sqrt(x*x + y*y + z*z); }
    void    normalize() {
        double norm = std::sqrt(x*x + y*y + z*z);
        x /= norm; y /= norm; z /= norm;
    }

    double& operator[](const int i)       { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }
    double  operator[](const int i) const { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }
    vec     operator+(const vec& other) const { return {other.x + x , other.y + y , other.z + z}; }
    vec     operator-(const vec& other) const { return {other.x - x , other.y - y , other.z - z}; }
    double  operator*(const vec& other) const { return other.x * x + other.y * y + other.z * z; }
    vec     operator^(const vec& other) const {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }
};

typedef vec<3> vec3;

#endif //GEOMETRY_H