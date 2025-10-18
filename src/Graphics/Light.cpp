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
    m_lightMove = true;
    m_lightModel->mesh.translation = position_;
    if (m_type == POINT_LIGHT) {
        m_position = position_;
    } else if (m_type == DIRECTIONAL_LIGHT) {
        m_position = position_;
        m_dirInfo->LightCamera.getCenter() = position_;
    }
}

void Light::setType(LIGHT_TYPE t) {
    m_lightMove = true;
    m_type = t;
    if (t == DIRECTIONAL_LIGHT) {
        if (!m_dirInfo) m_dirInfo.emplace();
        m_pointInfo.reset();
    } else if (t == POINT_LIGHT) {
        if (!m_pointInfo) m_pointInfo.emplace();
        m_dirInfo.reset();
    }
}

void Light::ProcessShadowMapIfNeeded(Scene &scene) {
    if (m_type == DIRECTIONAL_LIGHT && m_lightMove) {
        // Prepare shadow map size if not matching
        int lw = m_dirInfo->LightCamera.getWidth();
        int lh = m_dirInfo->LightCamera.getHeight();
        if (m_dirInfo->shadowMap.width() != lw || m_dirInfo->shadowMap.height() != lh) {
            m_dirInfo->shadowMap = TGAImage(lw, lh, TGAImage::RGBA);
        }

        Camera tmp = scene.camera;
        scene.camera = m_dirInfo->LightCamera;
        Rasterizer::get().pass(scene, ZTEST);
        scene.camera = tmp;

        Rasterizer::get().zBuffer_to_TGA(m_dirInfo->shadowMap);
        m_dirInfo->LightN = m_dirInfo->LightCamera.get_viewport_matrix(1)
                                            * m_dirInfo->LightCamera.get_projection_matrix()
                                            * m_dirInfo->LightCamera.get_view_matrix();
    } else if (m_type == POINT_LIGHT && m_lightMove) {
        Camera tmp = scene.camera;

        Camera lightCam(m_position + vec3{1, 0, 0}, m_position, {0, 1, 0}, 512, 512, 90.0, 1.0, 100.0);
        scene.camera = lightCam;

        Rasterizer::get().pass(scene, ZTEST);
        m_pointInfo->shadowMap[0] = TGAImage(512, 512, TGAImage::RGBA);
        Rasterizer::get().zBuffer_to_TGA(m_pointInfo->shadowMap[0]);
        m_pointInfo->LightN[0] = scene.camera.get_viewport_matrix(1)
                                        * scene.camera.get_projection_matrix()
                                        * scene.camera.get_view_matrix();

        scene.camera.set_toward_from_center(90, 0, 0);
        Rasterizer::get().pass(scene, ZTEST);
        m_pointInfo->shadowMap[1] = TGAImage(512, 512, TGAImage::RGBA);
        Rasterizer::get().zBuffer_to_TGA(m_pointInfo->shadowMap[1]);
        m_pointInfo->LightN[1] = scene.camera.get_viewport_matrix(1)
                                        * scene.camera.get_projection_matrix()
                                        * scene.camera.get_view_matrix();

        scene.camera.set_toward_from_center(180, 0, 0);
        Rasterizer::get().pass(scene, ZTEST);
        m_pointInfo->shadowMap[2] = TGAImage(512, 512, TGAImage::RGBA);
        Rasterizer::get().zBuffer_to_TGA(m_pointInfo->shadowMap[2]);
        m_pointInfo->LightN[2] = scene.camera.get_viewport_matrix(1)
                                        * scene.camera.get_projection_matrix()
                                        * scene.camera.get_view_matrix();

        scene.camera.set_toward_from_center(-90, 0, 0);
        Rasterizer::get().pass(scene, ZTEST);
        m_pointInfo->shadowMap[3] = TGAImage(512, 512, TGAImage::RGBA);
        Rasterizer::get().zBuffer_to_TGA(m_pointInfo->shadowMap[3]);
        m_pointInfo->LightN[3] = scene.camera.get_viewport_matrix(1)
                                        * scene.camera.get_projection_matrix()
                                        * scene.camera.get_view_matrix();

        scene.camera.set_toward_from_center(0, 89.9, 0);
        Rasterizer::get().pass(scene, ZTEST);
        m_pointInfo->shadowMap[4] = TGAImage(512, 512, TGAImage::RGBA);
        Rasterizer::get().zBuffer_to_TGA(m_pointInfo->shadowMap[4]);
        m_pointInfo->LightN[4] = scene.camera.get_viewport_matrix(1)
                                        * scene.camera.get_projection_matrix()
                                        * scene.camera.get_view_matrix();

        scene.camera.set_toward_from_center(0, -89.9, 0);
        Rasterizer::get().pass(scene, ZTEST);
        m_pointInfo->shadowMap[5] = TGAImage(512, 512, TGAImage::RGBA);
        Rasterizer::get().zBuffer_to_TGA(m_pointInfo->shadowMap[5]);
        m_pointInfo->LightN[5] = scene.camera.get_viewport_matrix(1)
                                        * scene.camera.get_projection_matrix()
                                        * scene.camera.get_view_matrix();

        scene.camera = tmp;
    }
    m_lightMove = false;
}

float Light::getVisibility(const vec4& world_pos) const {
    if (!m_haveShadow)
        return 1.0;

    float visibility = 1.0;

    if (m_type == POINT_LIGHT) {
        vec3 light_dir = normalize(world_pos.to_vec3_point() - m_position);
        int face_index;
        if (std::abs(light_dir.x) >= std::abs(light_dir.y) && std::abs(light_dir.x) >= std::abs(light_dir.z)) {
            face_index = (light_dir.x > 0) ? 0 : 2;
        } else if (std::abs(light_dir.y) >= std::abs(light_dir.x) && std::abs(light_dir.y) >= std::abs(light_dir.z)) {
            face_index = (light_dir.y > 0) ? 4 : 5;
        } else {
            face_index = (light_dir.z > 0) ? 1 : 3;
        }
        vec3 light_space_pos = (m_pointInfo->LightN[face_index] * world_pos).to_vec3_point();
        double z = light_space_pos.z;
        if (m_pointInfo->shadowMap[face_index].get(static_cast<int>(light_space_pos.x), static_cast<int>(light_space_pos.y)).to_double() > z + 0.005) {
            visibility = 0.0f; // 在阴影中
        } else {
            visibility = 1.0f; // 可见
        }
    }

    if (m_type == DIRECTIONAL_LIGHT) {
        vec3 light_space_pos = (m_dirInfo->LightN * world_pos).to_vec3_point();
        double z = light_space_pos.z;
        if (m_dirInfo->shadowMap.get(static_cast<int>(light_space_pos.x), static_cast<int>(light_space_pos.y)).to_double() > z + 0.005) {
            visibility = 0.0f; // 在阴影中
        } else {
            visibility = 1.0f; // 可见
        }
    }

    return visibility;
}
