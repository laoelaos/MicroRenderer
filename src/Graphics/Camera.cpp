//
// Created by laoe on 2025/10/12.
//

#include "Camera.h"

#include <algorithm>

Camera::Camera(const vec3& eye_, const vec3& center_, const vec3& up_, int w, int h,
               double fov_, double near_, double far_)
: m_eye(eye_), m_center(center_), m_up(up_), m_width(w), m_height(h),
  m_fov(fov_), m_aspect(static_cast<double>(w)/h), m_near(near_), m_far(far_) {}

mat4 Camera::get_view_matrix() const {
    vec3 z = normalize(m_eye - m_center);
    vec3 x = normalize(z ^ m_up);
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
            {1, 0, 0, -m_center.x},
            {0, 1, 0, -m_center.y},
            {0, 0, 1, -m_center.z},
            {0, 0, 0, 1}
        }
    };
    return rotate * translate;
}

mat4 Camera::get_projection_matrix() const {
    double top = m_near * std::tan(m_fov * M_PI / 360.0);
    double bottom = -top;
    double right = top * m_aspect;
    double left = -right;
    double near_cp = -m_near;
    double far_cp = -m_far;
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
    return {{{m_width/2., 0, 0, m_width/2.},
                        {0, m_height/2., 0, m_height/2.},
                        {0, 0, 1, 0},
                        {0, 0, 0, 1}}};
}

void Camera::updateRotate() {
    updateEuler();
    double yaw_rad = m_yaw * M_PI / 180.0;
    double pitch_rad = m_pitch * M_PI / 180.0;
    double roll_rad = m_roll * M_PI / 180.0;

    double x = cos(yaw_rad) * cos(pitch_rad);
    double y = sin(pitch_rad);
    double z = sin(yaw_rad) * cos(pitch_rad);

    m_eye = vec3{x, y, z} * norm(m_center - m_eye) + m_center;

    vec3 forward = normalize(m_center - m_eye);
    vec3 world_up{0, 1, 0};
    vec3 right = normalize(forward ^ world_up);
    if (norm(right) < 1e-6) {
        // forward 与 world_up 几乎平行时，换用另一参考向量避免退化
        right = normalize(forward ^ vec3{0, 0, 1});
    }
    vec3 up0 = normalize(right ^ forward);
    m_up = up0 * cos(roll_rad) + right * sin(roll_rad);

    m_aspect = static_cast<double>(m_width) / m_height;
}

void Camera::updateEuler() {
    while (m_yaw < -180.0) m_yaw += 360.0;
    while (m_yaw > 180.0) m_yaw -= 360.0;
    while (m_pitch < -90) m_pitch += 180.0;
    while (m_pitch > 90) m_pitch -= 180.0;
    while (m_roll < -180.0) m_roll += 360.0;
    while (m_roll > 180.0) m_roll -= 360.0;
}

void Camera::set_toward_from_center(double yaw, double pitch, double roll) {
    this->m_yaw = yaw;
    this->m_pitch = pitch;
    this->m_roll = roll;
    updateRotate();
}

void Camera::set_toward_from_point(vec3 point, double yaw, double pitch, double roll) {

}

void Camera::rotate_around_center(double yaw, double pitch, double roll) {
    this->m_yaw += yaw;
    this->m_pitch += pitch;
    this->m_roll += roll;
    updateRotate();
}

void Camera::rotate_around_point(vec3 point, double yaw, double pitch, double roll) {
    m_yaw += 180.0;
    m_pitch += 180.0;

    m_eye = m_center;
    m_center = point;
    rotate_around_center(yaw, pitch, roll);
    m_center = m_eye;
    m_eye = point;

    m_yaw -= 180.0;
    m_pitch -= 180.0;
    updateEuler();
}