//
// Created by laoe on 25-9-19.
//

#include "Graphics.h"

#include <algorithm>
#include <fstream>
#include <sstream>

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

//TODO: 是否有些繁琐，是否应当将输出方法迁移到各个类
Scene::Scene(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开场景文件: " + filename);
    }

    this->filename = filename;
    std::string line, section;
    std::string model_filename;

    camera = Camera();
    Light light;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;

        if (line.empty() || line[0] == '#') continue; // 跳过空行和注释

        if (line[0] == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }

        iss >> key;

        if (section == "Camera") {
            if (key == "eye") iss >> camera.eye.x >> camera.eye.y >> camera.eye.z;
            else if (key == "center") iss >> camera.center.x >> camera.center.y >> camera.center.z;
            else if (key == "up") iss >> camera.up.x >> camera.up.y >> camera.up.z;
            else if (key == "size") iss >> camera.width >> camera.height;
            else if (key == "fov") iss >> camera.fov;
            else if (key == "near_far") iss >> camera.near_ >> camera.far_;
            else if (key == "yaw_pitch_roll") iss >> camera.yaw >> camera.pitch >> camera.roll;
        }
        else if (section == "Light") {
            if (key == "color") {
                light = {};
                iss >> light.color.x >> light.color.y >> light.color.z;
            }

            else if (key == "position") iss >> light.position.x >> light.position.y >> light.position.z;
            else if (key == "intensity") iss >> light.intensity;

            else if (key == "LightCamera") light.LightCamera = Camera();
            else if (key == "eye") iss >> light.LightCamera->eye.x >> light.LightCamera->eye.y >> light.LightCamera->eye.z;
            else if (key == "center") iss >> light.LightCamera->center.x >> light.LightCamera->center.y >> light.LightCamera->center.z;
            else if (key == "up") iss >> light.LightCamera->up.x >> light.LightCamera->up.y >> light.LightCamera->up.z;
            else if (key == "size") iss >> light.LightCamera->width >> light.LightCamera->height;
            else if (key == "fov") iss >> light.LightCamera->fov;
            else if (key == "near_far") iss >> light.LightCamera->near_ >> light.LightCamera->far_;
            else if (key == "yaw_pitch_roll") iss >> light.LightCamera->yaw >> light.LightCamera->pitch >> light.LightCamera->roll;

            else if (key == "end") lights.push_back(light);
        }
        else if (section == "Model") {
            if (key == "path") {
                iss >> model_filename;
                models.emplace_back(model_filename);
                models.back().model_path = model_filename;
            }
            else if (key == "name") {
                std::string name;
                iss >> name;
                if (!models.empty()) models.back().name = name;
            }
            else if (key == "enable") {
                bool enable;
                iss >> enable;
                if (!models.empty()) models.back().enable = enable;
            }
            else if (key == "translation") {
                if (!models.empty()) iss >> models.back().translation.x >> models.back().translation.y >> models.back().translation.z;
            }
            else if (key == "rotation") {
                if (!models.empty()) iss >> models.back().rotation.x >> models.back().rotation.y >> models.back().rotation.z;
            }
            else if (key == "scale") {
                if (!models.empty()) iss >> models.back().scale.x >> models.back().scale.y >> models.back().scale.z;
            }
            else if (key == "texture") {
                std::string texture_path;
                iss >> texture_path;
                if (!models.empty() && !texture_path.empty()) {
                    models.back().material.load_texture(texture_path);
                }
            }
            else if (key == "normal_map") {
                std::string normal_map_path;
                iss >> normal_map_path;
                if (!models.empty() && !normal_map_path.empty()) {
                    models.back().material.load_normal_map(normal_map_path);
                }
            }
            else if (key == "diffuse_mapping") {
                bool diffuse_mapping;
                iss >> diffuse_mapping;
                if (!models.empty()) models.back().material.diffuse_mapping = diffuse_mapping;
            }
            else if (key == "normal_type") {
                int normal_type;
                iss >> normal_type;
                if (!models.empty()) models.back().material.normal_type = static_cast<NormalType>(normal_type);
            }
            else if (key == "shade_frequency") {
                int shade_freq;
                iss >> shade_freq;
                if (!models.empty()) models.back().material.shade_frequency = static_cast<ShadeFrequency>(shade_freq);
            }
            else if (key == "phong") {
                if (!models.empty()) {
                    iss >> models.back().material.properties.k_diffuse
                        >> models.back().material.properties.k_specular
                        >> models.back().material.properties.k_ambient
                        >> models.back().material.properties.p;
                }
            }
        }
    }
}

void Scene::save_path_file() const {
    save_path_file(filename);
}

void Scene::save_path_file(const std::string &filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("无法创建场景文件: " + filename);
    }

    file << "# MicroRenderer Scene File\n\n";

    file << "[Camera]\n";
    file << "eye " << camera.eye.x << " " << camera.eye.y << " " << camera.eye.z << "\n";
    file << "center " << camera.center.x << " " << camera.center.y << " " << camera.center.z << "\n";
    file << "up " << camera.up.x << " " << camera.up.y << " " << camera.up.z << "\n";
    file << "size " << camera.width << " " << camera.height << "\n";
    file << "fov " << camera.fov << "\n";
    file << "near_far " << camera.near_ << " " << camera.far_ << "\n";
    file << "yaw_pitch_roll " << camera.yaw << " " << camera.pitch << " " << camera.roll << "\n\n";

    for (const auto& light : lights) {
        file << "[Light]\n";
        file << "color " << light.color.x << " " << light.color.y << " " << light.color.z << "\n";
        file << "position " << light.position.x << " " << light.position.y << " " << light.position.z << "\n";
        file << "intensity " << light.intensity << "\n";
        if (light.LightCamera.has_value()) {
            file << "LightCamera" << '\n';
            file << "eye " << light.LightCamera.value().eye.x << " " << light.LightCamera.value().eye.y << " " << light.LightCamera.value().eye.z << "\n";
            file << "center " << light.LightCamera.value().center.x << " " << light.LightCamera.value().center.y << " " << light.LightCamera.value().center.z << "\n";
            file << "up " << light.LightCamera.value().up.x << " " << light.LightCamera.value().up.y << " " << light.LightCamera.value().up.z << "\n";
            file << "size " << light.LightCamera.value().width << " " << light.LightCamera.value().height << "\n";
            file << "fov " << light.LightCamera.value().fov << "\n";
            file << "near_far " << light.LightCamera.value().near_ << " " << light.LightCamera.value().far_ << "\n";
            file << "yaw_pitch_roll " << light.LightCamera.value().yaw << " " << light.LightCamera.value().pitch << " " << light.LightCamera.value().roll << "\n\n";
        } else {
            file << "end\n\n";
        }
    }

    for (const auto& model : models) {
        file << "[Model]\n";
        file << "path " << model.model_path << "\n";
        file << "name " << model.name << "\n";
        file << "enable " << model.enable << "\n";
        file << "translation " << model.translation.x << " " << model.translation.y << " " << model.translation.z << "\n";
        file << "rotation " << model.rotation.x << " " << model.rotation.y << " " << model.rotation.z << "\n";
        file << "scale " << model.scale.x << " " << model.scale.y << " " << model.scale.z << "\n";

        if (!model.material.texture_path.empty()) {
            file << "texture " << model.material.texture_path << "\n";
        }

        if (!model.material.normal_map_path.empty()) {
            file << "normal_map " << model.material.normal_map_path << "\n";
        }

        file << "diffuse_mapping " << model.material.diffuse_mapping << "\n";
        file << "normal_type " << static_cast<int>(model.material.normal_type) << "\n";
        file << "shade_frequency " << static_cast<int>(model.material.shade_frequency) << "\n";
        file << "phong "
             << model.material.properties.k_diffuse << " "
             << model.material.properties.k_specular << " "
             << model.material.properties.k_ambient << " "
             << model.material.properties.p << "\n\n";
    }

    file.close();
}
