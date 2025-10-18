//
// Created by laoe on 2025/10/12.
//

#include "Scene.h"

#include <sstream>

Scene::Scene(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开场景文件: " + filename);
    }

    this->filename = filename;
    std::string line, section;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;

        if (line.empty() || line[0] == '#') continue; // 跳过空行和注释

        if (line[0] == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
        }

        if (section == "Camera") {
            LoadCamera(file);
        }
        else if (section == "POINT_LIGHT") {
            LoadLight(file, POINT_LIGHT);
        }
        else if (section == "DIRECTIONAL_LIGHT") {
            LoadLight(file, DIRECTIONAL_LIGHT);
        }
        else if (section == "Model") {
            LoadModel(file);
        }
    }
}

void Scene::SetMove() {
    for (Light& light : lights) {
        light.m_lightMove = true;
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

    SaveCamera(file);

    for (const auto& light : lights) {
        SaveLight(file, light);
    }

    for (const auto& model : models) {
        SaveModel(file, model);
    }

    file.close();
}

void Scene::SaveModel(std::ofstream& file, const Model &model) const {
    file << "[Model]\n";
    file << "path " << model.model_path << "\n";
    file << "name " << model.name << "\n";
    file << "enable " << model.enable << "\n";
    file << "translation " << model.mesh.translation.x << " " << model.mesh.translation.y << " " << model.mesh.translation.z << "\n";
    file << "rotation " << model.mesh.rotation.x << " " << model.mesh.rotation.y << " " << model.mesh.rotation.z << "\n";
    file << "scale " << model.mesh.scale.x << " " << model.mesh.scale.y << " " << model.mesh.scale.z << "\n";

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
    file << "end\n\n";
}

void Scene::LoadModel(std::ifstream& file) {
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key, model_filename;
        iss >> key;

        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[' && line.back() == ']') throw std::runtime_error("wrong camera format");

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
            if (!models.empty()) iss >> models.back().mesh.translation.x >> models.back().mesh.translation.y >> models.back().mesh.translation.z;
        }
        else if (key == "rotation") {
            if (!models.empty()) iss >> models.back().mesh.rotation.x >> models.back().mesh.rotation.y >> models.back().mesh.rotation.z;
        }
        else if (key == "scale") {
            if (!models.empty()) iss >> models.back().mesh.scale.x >> models.back().mesh.scale.y >> models.back().mesh.scale.z;
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
            if (!models.empty()) models.back().material.normal_type = static_cast<NormalMapType>(normal_type);
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
        if (key == "end") return;
    }
}

void Scene::SaveCamera(std::ofstream& file) const {
    file << "[Camera]\n";
    file << "center " << camera.m_center.x << " " << camera.m_center.y << " " << camera.m_center.z << "\n";
    file << "size " << camera.m_width << " " << camera.m_height << "\n";
    file << "fov " << camera.m_fov << "\n";
    file << "near_far " << camera.m_near << " " << camera.m_far << "\n";
    file << "yaw_pitch_roll " << camera.m_yaw << " " << camera.m_pitch << " " << camera.m_roll << "\n";
    file << "end\n\n";
}

void Scene::LoadCamera(std::ifstream& file) {
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[' && line.back() == ']') throw std::runtime_error("wrong camera format");

        if (key == "eye") iss >> camera.m_eye.x >> camera.m_eye.y >> camera.m_eye.z;
        else if (key == "center") iss >> camera.m_center.x >> camera.m_center.y >> camera.m_center.z;
        else if (key == "up") iss >> camera.m_up.x >> camera.m_up.y >> camera.m_up.z;
        else if (key == "size") iss >> camera.m_width >> camera.m_height;
        else if (key == "fov") iss >> camera.m_fov;
        else if (key == "near_far") iss >> camera.m_near >> camera.m_far;
        else if (key == "yaw_pitch_roll") iss >> camera.m_yaw >> camera.m_pitch >> camera.m_roll;
        if (key == "end") { camera.updateRotate(); return; }
    }
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
        file << "eye" << light.m_dirInfo->LightCamera.m_eye.x << " " << light.m_dirInfo->LightCamera.m_eye.y << " " << light.m_dirInfo->LightCamera.m_eye.z << "\n";
        file << "center" << light.m_dirInfo->LightCamera.m_center.x << " "<< light.m_dirInfo->LightCamera.m_center.y << " " << light.m_dirInfo->LightCamera.m_center.z << "\n";
        file << "up" << light.m_dirInfo->LightCamera.m_up.x << " " << light.m_dirInfo->LightCamera.m_up.y << " " << light.m_dirInfo->LightCamera.m_up.z << "\n";
        file << "size" << light.m_dirInfo->LightCamera.m_width << " " << light.m_dirInfo->LightCamera.m_height << "\n";
        file << "fov" << light.m_dirInfo->LightCamera.m_fov << "\n";
        file << "near_far" << light.m_dirInfo->LightCamera.m_near << " " << light.m_dirInfo->LightCamera.m_far << "\n";
        file << "yaw_pitch_roll" << light.m_dirInfo->LightCamera.m_yaw << " " << light.m_dirInfo->LightCamera.m_pitch << " " << light.m_dirInfo->LightCamera.m_roll << "\n";
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
        else if (key == "eye" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.m_eye.x >> light.m_dirInfo->LightCamera.m_eye.y >> light.m_dirInfo->LightCamera.m_eye.z;
        else if (key == "center" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.m_center.x >> light.m_dirInfo->LightCamera.m_center.y >> light.m_dirInfo->LightCamera.m_center.z;
        else if (key == "up" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.m_up.x >> light.m_dirInfo->LightCamera.m_up.y >> light.m_dirInfo->LightCamera.m_up.z;
        else if (key == "size" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.m_width >> light.m_dirInfo->LightCamera.m_height;
        else if (key == "fov" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.m_fov;
        else if (key == "near_far" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.m_near >> light.m_dirInfo->LightCamera.m_far;
        else if (key == "yaw_pitch_roll" && light.m_dirInfo) iss >> light.m_dirInfo->LightCamera.m_yaw >> light.m_dirInfo->LightCamera.m_pitch >> light.m_dirInfo->LightCamera.m_roll;
        else if (key == "end") {
            models.emplace_back(CUBE);
            light.m_lightModel = &models.back();
            light.m_lightModel->name = "light_model";
            light.m_lightModel->necessary = true;
            light.m_lightModel->mesh.scale = vec3{0.05, 0.05, 0.05};
            light.m_lightModel->mesh.translation = light.m_position;
            light.m_lightModel->material.properties = {0.0, 0.0, 1.0, 1};
            return;
        }
    }
}