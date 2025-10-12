//
// Created by laoe on 2025/10/12.
//

#include "Scene.h"

#include <sstream>

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