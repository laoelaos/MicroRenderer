//
// Created by laoe on 25-9-4.
//

#ifndef GEOMETRY_H
#define GEOMETRY_H
#include <cassert>
#include <ostream>
#endif //GEOMETRY_H

template<int n> struct vec {
    double data[n] = {0};
    double& operator[](int i) { assert(i>=0&&i<n); return data[i]; }
    double  operator[](int i) const { assert(i>=0&&i<n); return data[i]; }
};

template<int n> std::ostream& operator<<(std::ostream& out, const vec<n>& v) {
    for (int i=0; i<n; i++) out << v[i] << " ";
    return out;
}

template<> struct vec<3> {
    double x = 0, y = 0, z = 0;
    double& operator[](const int i)       { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }
    double  operator[](const int i) const { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }
};

typedef vec<3> vec3;
