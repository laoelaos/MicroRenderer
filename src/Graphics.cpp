//
// Created by laoe on 25-9-19.
//

#include "Graphics.h"

#include <algorithm>

#include "Geometry.h"


Camera::Camera(const vec3& eye_, const vec3& center_, const vec3& up_, int w, int h,
               double fov_, double near_, double far_)
: eye(eye_), center(center_), up(up_), width(w), height(h),
  fov(fov_), aspect(static_cast<double>(w)/h), near_(near_), far_(far_) {}

mat4 Camera::get_view_matrix() const {
    vec3 z = normalize(eye - center);
    vec3 x = normalize(z ^ up);
    vec3 y = normalize(x ^ z);
    mat4 rotate{
        {
            {x.x, x.y, x.z, 0},
            {y.x, y.y, y.z, 0},
            {-z.x, -z.y, -z.z, 0},
            {0, 0, 0, 1}
        }
    };
    mat4 translate{
        {
            {1, 0, 0, -center.x},
            {0, 1, 0, -center.y},
            {0, 0, 1, -center.z},
            {0, 0, 0, 1}
        }
    };
    return rotate * translate;
}

mat4 Camera::get_projection_matrix() const {
    double top = near_ * std::tan(fov * M_PI / 360.0);
    double bottom = -top;
    double right = top * aspect;
    double left = -right;
    double near_cp = -near_;
    double far_cp = -far_;
    mat4 translate{
        {
            {1, 0, 0, -(left + right) / 2},
            {0, 1, 0, -(top + bottom) / 2},
            {0, 0, 1, -(near_cp + far_cp) / 2},
            {0, 0, 0, 1}
        }
    };
    mat4 scale{
        {
            {2 / (right - left), 0, 0, 0},
            {0, 2 / (top - bottom), 0, 0},
            {0, 0, 2 / (near_cp - far_cp), 0},
            {0, 0, 0, 1}
        }
    };
    mat4 pers{
        {
            {near_cp, 0, 0, 0},
            {0, near_cp, 0, 0},
            {0, 0, near_cp + far_cp, -near_cp * far_cp},
            {0, 0, 1, 0}
        }
    };
    return scale * translate * pers;
}

mat4 Camera::get_viewport_matrix() const {
    return {{{width/2., 0, 0, width/2.},
                        {0, height/2., 0, height/2.},
                        {0, 0, 1, 0},
                        {0, 0, 0, 1}}};
}

void Camera::update() {
    double yaw_rad = yaw * M_PI / 180.0;
    double pitch_rad = pitch * M_PI / 180.0;
    double roll_rad = roll * M_PI / 180.0;

    pitch_rad = std::clamp(pitch_rad, -M_PI/2.0 + 0.001, M_PI/2.0 - 0.001);

    double x = cos(yaw_rad) * cos(pitch_rad);
    double y = sin(pitch_rad);
    double z = sin(yaw_rad) * cos(pitch_rad);

    eye = vec3{x, y, z} * norm(center - eye) + center;

    vec3 forward = normalize(center - eye);
    vec3 world_up{0, 1, 0};
    vec3 right = normalize(forward ^ world_up);
    if (norm(right) < 1e-6) {
        // forward 与 world_up 几乎平行时，换用另一参考向量避免退化
        right = normalize(forward ^ vec3{0, 0, 1});
    }
    vec3 up0 = normalize(right ^ forward);
    up = up0 * cos(roll_rad) + right * sin(roll_rad);

    aspect = static_cast<double>(width) / height;
}

void Camera::set_toward_from_center(double yaw, double pitch, double roll) {
    this->yaw = yaw;
    this->pitch = pitch;
    this->roll = roll;
    update();
}

void Camera::set_toward_from_eye(double yaw, double pitch, double roll) {
    std::swap(eye, center);
    set_toward_from_center(yaw, pitch, roll);
    std::swap(eye, center);
}

void Camera::rotate_around_center(double yaw, double pitch, double roll) {
    this->yaw += yaw;
    this->pitch += pitch;
    this->roll += roll;
    update();
}

void Camera::rotate_around_eye(double yaw, double pitch, double roll) {
    std::swap(eye, center);
    rotate_around_center(yaw, pitch, roll);
    std::swap(eye, center);
}

Light::Light(const vec3& color_, const vec3& position_, double intensity_)
: color(color_), position(position_), intensity(intensity_) {}

vec3 Light::get_illumination_at(const vec3& point) const {
    return color * intensity / norm2(point - position);
}