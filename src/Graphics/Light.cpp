//
// Created by laoe on 2025/10/12.
//

#include "Light.h"
#include "Scene.h"
#include "../Rasterizer.h"

Light::Light(const vec3& color_, const vec3& position_, double intensity_)
: m_color(color_), m_position(position_), m_intensity(intensity_) {}

Light::Light(const vec3 &color_, const vec3 &position_, double intensity_, Camera &lightCamera)
    : m_color(color_), m_position(position_), m_intensity(intensity_) {
    m_dirInfo = DirectionalLightInfo();
    m_dirInfo->LightCamera = lightCamera;
    m_type = DIRECTIONAL_LIGHT;
}

vec3 Light::get_illumination_at(const vec3& point) const {
    return m_color * m_intensity / norm2(point - m_position);
}

void Light::setPosition(const vec3 &position_) {
    if (m_type == POINT_LIGHT) {
        m_position = position_;
    } else if (m_type == DIRECTIONAL_LIGHT) {
        m_position = position_;
        m_dirInfo->LightCamera.center = position_;
        m_dirInfo->lightMove = true;
    }
}

void Light::setType(LIGHT_TYPE t) {
    m_type = t;
    if (t == DIRECTIONAL_LIGHT) {
        if (!m_dirInfo) m_dirInfo.emplace();
    } else {
        m_dirInfo.reset();
    }
}

void Light::ProcessShadowMapIfNeeded(Scene &scene) {
    if (m_type == DIRECTIONAL_LIGHT && m_dirInfo->lightMove) {
        // Prepare shadow map size if not matching
        int lw = m_dirInfo->LightCamera.width;
        int lh = m_dirInfo->LightCamera.height;
        if (m_dirInfo->shadowMap.width() != lw || m_dirInfo->shadowMap.height() != lh) {
            m_dirInfo->shadowMap = TGAImage(lw, lh, TGAImage::RGB);
        }

        Camera tmp = scene.camera;
        scene.camera = m_dirInfo->LightCamera;
        Rasterizer::get().rasterize(scene, ZTEST);
        scene.camera = tmp;

        Rasterizer::get().zBuffer_to_TGA(m_dirInfo->shadowMap);
        m_dirInfo->LightN = m_dirInfo->LightCamera.get_viewport_matrix()
                                            * m_dirInfo->LightCamera.get_projection_matrix()
                                            * m_dirInfo->LightCamera.get_view_matrix();
        m_dirInfo->lightMove = false;
    }
}

float Light::getVisibility(const vec4& world_pos) const {
    if (m_type == POINT_LIGHT) {
        return 1.0f;
    }

    if (m_type == DIRECTIONAL_LIGHT) {
        if (!m_dirInfo->haveShadow)
            return 1.0;

        vec3 light_space_pos = (m_dirInfo->LightN * world_pos).to_vec3_point();
        double z = light_space_pos.z;
        if (m_dirInfo->shadowMap.get(static_cast<int>(light_space_pos.x), static_cast<int>(light_space_pos.y)).to_double() > z + 0.005) {
            return 0.0f; // 在阴影中
        }
        return 1.0f; // 可见
    }

    return 1.0;
}
