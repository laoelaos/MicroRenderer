//
// Created by laoe on 2025/10/14.
//

#include "ConfigGui.h"

void ConfigGui::ConfigLight(Light &light) {
    std::array<float, 3> light_position = to_float_array(light.m_position);
    std::array<float, 3> light_color = to_float_array(light.m_color);
    auto light_intensity = static_cast<float>(light.m_intensity);

    if (ImGui::InputFloat3("位置", light_position.data(), "%.1f")) {
        light.setPosition(float_array_to_vec(light_position));
        if (light.getType() == DIRECTIONAL_LIGHT) {
            light.m_dirInfo->lightMove = true;
        }
    }
    if (ImGui::ColorEdit3("颜色", light_color.data()))
        light.m_color = {light_color[0], light_color[1], light_color[2]};
    if (ImGui::InputFloat("光强", &light_intensity, 10.0f, 100.0f, "%.1f"))
        light.m_intensity = std::max(0.0f, light_intensity);

}
