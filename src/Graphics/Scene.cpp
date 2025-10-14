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
    Light light; // 当前在解析的光
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
        else if (section == "POINT_LIGHT") {
            LoadLight(file, POINT_LIGHT);
        }
        else if (section == "DIRECTIONAL_LIGHT") {
            LoadLight(file, DIRECTIONAL_LIGHT);
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
        SaveLight(file, light);
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

void Scene::SaveModel(std::ofstream& file, const Model &model) const {
}

void Scene::LoadModel(std::ifstream& file) {
}

void Scene::SaveCamera(std::ofstream& file) const {
}

void Scene::LoadCamera(std::ifstream& file) {

}

void Scene::SaveLight(std::ofstream& file, const Light& light) const {
    if (light.getType() == POINT_LIGHT) {
        file << "[POINT_LIGHT]\n";
    } else if (light.getType() == DIRECTIONAL_LIGHT) {
        file << "[DIRECTIONAL_LIGHT]\n";
    }

    file << "color" << light.m_color.x << " " << light.m_color.y << " " << light.m_color.z << "\n";
    file << "position" << light.m_position.x << " " << light.m_position.y << " " << light.m_position.z << "\n";
    file << "intensity" << light.m_intensity << "\n";
    if (light.getType() == DIRECTIONAL_LIGHT) {
        file << "eye" << light.m_dirInfo->LightCamera.eye.x << " " << light.m_dirInfo->LightCamera.eye.y << " " << light.m_dirInfo->LightCamera.eye.z << "\n";
        file << "center" << light.m_dirInfo->LightCamera.center.x << " "<< light.m_dirInfo->LightCamera.center.y << " " << light.m_dirInfo->LightCamera.center.z << "\n";
        file << "up" << light.m_dirInfo->LightCamera.up.x << " " << light.m_dirInfo->LightCamera.up.y << " " << light.m_dirInfo->LightCamera.up.z << "\n";
        file << "size" << light.m_dirInfo->LightCamera.width << " " << light.m_dirInfo->LightCamera.height << "\n";
        file << "fov" << light.m_dirInfo->LightCamera.fov << "\n";
        file << "near_far" << light.m_dirInfo->LightCamera.near_ << " " << light.m_dirInfo->LightCamera.far_ << "\n";
        file << "yaw_pitch_roll" << light.m_dirInfo->LightCamera.yaw << " " << light.m_dirInfo->LightCamera.pitch << " " << light.m_dirInfo->LightCamera.roll << "\n";
    }
    file << "end\n\n";
}

void Scene::LoadLight(std::ifstream& file, LIGHT_TYPE type) {
    lights.emplace_back();
    Light& light = lights.back();
    light.setType(type);

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[' && line.back() == ']') throw std::runtime_error("wrong light format");

        if (key == "color") iss >> light.m_color.x >> light.m_color.y >> light.m_color.z;
        else if (key == "position") iss >> light.m_position.x >> light.m_position.y >> light.m_position.z;
        else if (key == "intensity") iss >> light.m_intensity;
        else if (key == "eye" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.eye.x >> light.m_dirInfo->LightCamera.eye.y >> light.m_dirInfo->LightCamera.eye.z;
        else if (key == "center" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.center.x >> light.m_dirInfo->LightCamera.center.y >> light.m_dirInfo->LightCamera.center.z;
        else if (key == "up" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.up.x >> light.m_dirInfo->LightCamera.up.y >> light.m_dirInfo->LightCamera.up.z;
        else if (key == "size" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.width >> light.m_dirInfo->LightCamera.height;
        else if (key == "fov" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.fov;
        else if (key == "near_far" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.near_ >> light.m_dirInfo->LightCamera.far_;
        else if (key == "yaw_pitch_roll" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.yaw >> light.m_dirInfo->LightCamera.pitch >> light.m_dirInfo->LightCamera.roll;
        else if (key == "end") return;
    }
}