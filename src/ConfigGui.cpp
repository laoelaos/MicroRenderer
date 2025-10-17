//
// Created by laoe on 2025/10/14.
//

#include "imgui.h"
#include "imgui_stdlib.h"
#include "ConfigGui.h"

#include <algorithm>

#include "Rasterizer.h"

ConfigGui& ConfigGui::get() {
    static ConfigGui instance;
    return instance;
}

void ConfigGui::LaunchConfig(Scene &scene) {
    ImGui::Begin("场景设置");

    ConfigCamera(scene.camera);

    if (ImGui::CollapsingHeader("光照设置")) {
        ImGui::Text("光源数量: %zu", scene.lights.size());

        if (ImGui::Button("添加光源"))
            scene.lights.emplace_back();

        ImGui::Separator();
        for (size_t i = 0; i < scene.lights.size(); i++) {
            ImGui::PushID(static_cast<int>(i));

            std::string header = "光源 " + std::to_string(i + 1);
            if (ImGui::TreeNode(header.c_str())) {

                ConfigLight(scene.lights[i]);

                if (scene.lights.size() > 1) {
                    ImGui::SameLine();
                    if (ImGui::Button("删除")) {
                        scene.lights.erase(scene.lights.begin() + static_cast<int64_t>(i));
                        ImGui::TreePop();
                        ImGui::PopID();
                        break;
                    }
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    if (ImGui::CollapsingHeader("模型管理")) {
        ImGui::Text("场景中的模型个数: %zu", scene.models.size());

        if (ImGui::Button("添加新模型"))
            scene.models.emplace_back();

        ImGui::Separator();

        std::vector<const char*> model_names;
        for (auto & model : scene.models)
            model_names.push_back(model.name.c_str());

        if (!model_names.empty()) {
            ImGui::Combo("选择模型", &m_SelectedModel, model_names.data(), static_cast<int>(model_names.size()));

            if (m_SelectedModel >= 0 && m_SelectedModel < static_cast<int>(scene.models.size())) {
                Model& model = scene.models[m_SelectedModel];

                const char* default_mesh[] = {"正方体", "球体", "平面", "取消"};
                ImGui::Combo("使用默认模型", &m_DefaultModel, default_mesh, IM_ARRAYSIZE(default_mesh));
                ImGui::SameLine();
                if (ImGui::Button("应用")) {
                    scene.SetMove();
                    if (m_DefaultModel < 3) {
                        model.mesh = Mesh(static_cast<DefaultMesh>(m_DefaultModel));
                    } else if (m_DefaultModel == 3) {
                        model.mesh.LoadFromFile(model.model_path);
                    }
                    m_DefaultModel = -1; // 重置选择
                }

                if (ConfigModel(model)) {
                    scene.SetMove();
                }

                ImGui::Separator();
                if (scene.models.size() > 1) {  // 至少保留一个模型
                    if (ImGui::Button("删除模型")) {
                        scene.models.erase(scene.models.begin() + m_SelectedModel);
                        m_SelectedModel = std::min(m_SelectedModel, static_cast<int>(scene.models.size()) - 1);
                    }
                }
            }
        }
    }

    ImGui::End();
}

bool ConfigGui::ConfigCamera(Camera& camera) {
    if (ImGui::CollapsingHeader("相机设置")) {
        std::array<float, 3> eye_pos = to_float_array(camera.m_eye);
        std::array<float, 3> center_pos = to_float_array(camera.m_center);
        std::array<float, 3> up = to_float_array(camera.m_up);
        auto fov = static_cast<float>(camera.m_fov);
        auto near_plane = static_cast<float>(camera.m_near);
        auto far_plane = static_cast<float>(camera.m_far);
        auto yaw = static_cast<float>(camera.m_yaw);
        auto pitch = static_cast<float>(camera.m_pitch);
        auto roll = static_cast<float>(camera.m_roll);
        auto length = static_cast<float>(norm(camera.m_eye - camera.m_center));

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("远平面应大于近平面");

        bool view_changed = false;
        if (ImGui::SliderFloat("水平旋转 (Yaw)", &yaw, -180.0f, 180.0f, "%.1f°"))
            view_changed = true;
        if (ImGui::SliderFloat("垂直旋转 (Pitch)", &pitch, -89.0f, 89.0f, "%.1f°"))
            view_changed = true;
        if (ImGui::SliderFloat("滚转 (roll)", &roll, -180.0f, 180.0f, "%.1f°"))
            view_changed = true;
        if (view_changed) {
            camera.set_toward_from_center(yaw, pitch, roll);
        }
        if (ImGui::InputFloat3("相机位置", center_pos.data(), "%.2f"))
            camera.m_center = float_array_to_vec(center_pos);
        ImGui::BeginDisabled();
        ImGui::InputFloat3("观察目标点", eye_pos.data(), "%.2f");
        ImGui::InputFloat3("相机上方向", up.data(), "%.2f");
        ImGui::EndDisabled();

        if (ImGui::InputFloat("视场角FOV", &fov, 1.0f, 5.0f, "%.1f"))
            camera.m_fov = fov;
        if (ImGui::InputFloat("近平面", &near_plane, 0.1f, 0.5f, "%.2f"))
            camera.m_near = near_plane;
        if (ImGui::InputFloat("远平面", &far_plane, 0.1f, 0.5f, "%.2f"))
            camera.m_far = far_plane;
        if (ImGui::InputFloat("相机与目标点距离", &length, 0.1f, 0.5f, "%.2f")) {
            vec3 direction = normalize(camera.m_eye - camera.m_center);
            camera.m_center = camera.m_eye + direction * length;
        }

        if (ImGui::Button("重置相机"))
            camera = {};
    }
    return true;
}

void ConfigGui::ConfigLight(Light& light) {
    std::array<float, 3> light_position = to_float_array(light.m_position);
    std::array<float, 3> light_color = to_float_array(light.m_color);
    auto light_intensity = static_cast<float>(light.m_intensity);

    const char* light_types[] = {"点光源", "方向光"};
    int lightType = light.getType();
    if (ImGui::Combo("光源类型", &lightType, light_types, IM_ARRAYSIZE(light_types))) {
        light.setType(lightType == 0 ? POINT_LIGHT : DIRECTIONAL_LIGHT);
    }

    if (lightType == DIRECTIONAL_LIGHT) {
        ImGui::Checkbox("启用阴影", &light.m_dirInfo->haveShadow);
        ImGui::Text("光源位置即为光照方向");
        ImGui::Text("光照方向: (%.1f, %.1f, %.1f)", -light.m_position.x, -light.m_position.y, -light.m_position.z);
    }

    if (ImGui::InputFloat3("位置", light_position.data(), "%.1f")) {
        light.setPosition(float_array_to_vec(light_position));
        if (lightType == DIRECTIONAL_LIGHT) {
            light.m_dirInfo->lightMove = true;
        }
    }

    if (ImGui::ColorEdit3("颜色", light_color.data()))
        light.m_color = {light_color[0], light_color[1], light_color[2]};
    if (ImGui::InputFloat("光强", &light_intensity, 10.0f, 100.0f, "%.1f"))
        light.m_intensity = std::max(0.0f, light_intensity);

    if (lightType == DIRECTIONAL_LIGHT) {
        ImGui::Text("光照相机设置:");
        ImGui::InputInt("宽度", &light.m_dirInfo->LightCamera.m_width);
        ImGui::InputInt("高度", &light.m_dirInfo->LightCamera.m_height);

        auto fov = static_cast<float>(light.m_dirInfo->LightCamera.m_fov);
        auto near_plane = static_cast<float>(light.m_dirInfo->LightCamera.m_near);
        auto far_plane = static_cast<float>(light.m_dirInfo->LightCamera.m_far);
        if (ImGui::InputFloat("视场角FOV", &fov, 1.0f, 5.0f, "%.1f"))
            light.m_dirInfo->LightCamera.m_fov = fov;
        if (ImGui::InputFloat("近平面", &near_plane, 0.1f, 0.5f, "%.2f"))
            light.m_dirInfo->LightCamera.m_near = near_plane;
        if (ImGui::InputFloat("远平面", &far_plane, 0.1f, 0.5f, "%.2f"))
            light.m_dirInfo->LightCamera.m_far = far_plane;



        if (ImGui::Button("重置光照相机")) {
            light.m_dirInfo->LightCamera.m_eye = light.m_position;
            light.m_dirInfo->lightMove = true;
        }
    }
}

bool ConfigGui::ConfigModel(Model& model) {
    if (ImGui::TreeNode("模型基础设置")) {
        ImGui::Checkbox("启用模型", &model.enable);

        ImGui::InputText("模型名称", &model.name);
        if (ImGui::InputText("模型文件路径", &model.model_path))
            model.mesh.LoadFromFile(model.model_path);
        if (ImGui::InputText("漫反射贴图路径", &model.material.normal_map_path))
            model.material.load_normal_map();
        if (ImGui::InputText("法线贴图路径", &model.material.texture_path))
            model.material.load_texture();

        auto k_ambient = static_cast<float>(model.material.properties.k_ambient);
        auto k_diffuse = static_cast<float>(model.material.properties.k_diffuse);
        auto k_specular = static_cast<float>(model.material.properties.k_specular);
        int p = model.material.properties.p;
        if (ImGui::InputFloat("环境光系数", &k_ambient, 0.05f, 0.1f, "%.2f"))
            model.material.properties.k_ambient = std::clamp(static_cast<double>(k_ambient), 0.0, 1.0);
        if (ImGui::InputFloat("漫反射系数", &k_diffuse, 0.05f, 0.1f, "%.2f"))
            model.material.properties.k_diffuse = std::clamp(static_cast<double>(k_diffuse), 0.0, 1.0);
        if (ImGui::InputFloat("镜面反射系数", &k_specular, 0.005f, 0.01f, "%.3f"))
            model.material.properties.k_specular = std::clamp(static_cast<double>(k_specular), 0.0, 1.0);
        if (ImGui::InputInt("光泽度", &p, 10, 50))
            model.material.properties.p = std::clamp(p, 1, 1000);

        ImGui::Checkbox("使用贴图", &model.material.diffuse_mapping);

        const char *normal_types[] = {"全局法线贴图", "切线空间法线贴图"};
        int normal_type_idx = model.material.normal_type == GLOBAL ? 0 : 1;
        if (ImGui::Combo("法线类型", &normal_type_idx, normal_types, IM_ARRAYSIZE(normal_types))) {
            model.material.normal_type = normal_type_idx == 0 ? GLOBAL : TANGENT;
        }

        const char *shade_frequencies[] = {"每顶点着色", "每片段着色"};
        int shade_freq_idx = model.material.shade_frequency == PER_VERTEX ? 0 : 1;
        if (ImGui::Combo("着色频率", &shade_freq_idx, shade_frequencies, IM_ARRAYSIZE(shade_frequencies))) {
            model.material.shade_frequency = shade_freq_idx == 0 ? PER_VERTEX : PER_FRAGMENT;
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("模型变换")) {
        bool changed = false;
        std::array<float, 3> translation = to_float_array(model.mesh.translation);
        if (ImGui::InputFloat3("位移", translation.data(), "%.2f")) {
            changed = true;
            model.mesh.translation = float_array_to_vec(translation);
        }

        std::array<float, 3> rotation_degrees = to_float_array(model.mesh.rotation); //角度制
        if (ImGui::SliderFloat("X轴旋转", &rotation_degrees[0], -180.0f, 180.0f, "%.1f°")) {
            changed = true;
            model.mesh.rotation = float_array_to_vec(rotation_degrees);
        }
        if (ImGui::SliderFloat("Y轴旋转", &rotation_degrees[1], -180.0f, 180.0f, "%.1f°")) {
            changed = true;
            model.mesh.rotation = float_array_to_vec(rotation_degrees);
        }
        if (ImGui::SliderFloat("Z轴旋转", &rotation_degrees[2], -180.0f, 180.0f, "%.1f°")) {
            changed = true;
            model.mesh.rotation = float_array_to_vec(rotation_degrees);
        }

        std::array<float, 3> scale = to_float_array(model.mesh.scale);
        if (ImGui::InputFloat3("缩放", scale.data(), "%.2f")) {
            changed = true;
            model.mesh.scale = {
                std::max(0.01, static_cast<double>(scale[0])),
                std::max(0.01, static_cast<double>(scale[1])),
                std::max(0.01, static_cast<double>(scale[2]))
            };
        }

        ImGui::Separator();

        static float uniform_scale = 1.0f;
        ImGui::SliderFloat("统一缩放", &uniform_scale, 0.1f, 5.0f, "%.2f");
        ImGui::SameLine();
        if (ImGui::Button("应用统一缩放")) {
            changed = true;
            model.mesh.scale = {uniform_scale, uniform_scale, uniform_scale};
        }

        if (ImGui::Button("重置所有变换")) {
            changed = true;
            model.mesh.translation = {0, 0, 0};
            model.mesh.rotation = {0, 0, 0};
            model.mesh.scale = {1, 1, 1};
            uniform_scale = 1.0f;
        }

        ImGui::TreePop();
        return changed;
    }
    return false;
}

