//
// Created by laoe on 2025/10/12.
//

#include "Light.h"
#include "Scene.h"
#include "ImageUtils.h"
#include "../Rasterizer.h"

Light::Light() { setType(LightType_POINT); }

Light::Light(const vec3& color_, const vec3& position_, double intensity_)
: m_color(color_), m_position(position_), m_intensity(intensity_) { setType(LightType_POINT); }

Light::Light(const vec3 &color_, const vec3 &position_, double intensity_, Camera &lightCamera)
    : m_color(color_), m_position(position_), m_intensity(intensity_) {
    setType(LightType_DIRECTIONAL);
    m_dirInfo->LightCamera = lightCamera;
}

void Light::setPosition(const vec3 &position_) {
    m_lightMove = true;
    m_lightMesh->translation = position_;
    if (m_type == LightType_POINT) {
        m_position = position_;
    } else if (m_type == LightType_DIRECTIONAL) {
        m_dirInfo->LightCamera.Move(position_ - m_position);
        m_position = position_;
    }
}

void Light::setType(LightType t) {
    m_lightMove = true;
    m_type = t;
    if (t == LightType_DIRECTIONAL) {
        if (!m_dirInfo) {
            m_dirInfo.emplace();
            m_dirInfo->shadowMap = std::make_shared<FlatTexture>();
            m_dirInfo->LightCamera = Camera{{0, 0, 0}, m_position, {0,1,0}, 512, 512, 45.0, 1.0, 100.0};
        }
        m_pointInfo.reset();
    } else if (t == LightType_POINT) {
        if (!m_pointInfo) {
            m_pointInfo.emplace();
            m_pointInfo->shadowMap = std::make_shared<SixFacesCubeTexture>();
        }
        m_dirInfo.reset();
    }
}

void Light::ProcessShadowMapIfNeeded(Scene &scene) {
    if (m_type == LightType_DIRECTIONAL && m_lightMove) {
        // Prepare shadow map size if not matching
        int lw = m_dirInfo->LightCamera.getWidth();
        int lh = m_dirInfo->LightCamera.getHeight();
        if (!m_dirInfo->shadowMap->GetData() || m_dirInfo->shadowMap->GetData()->GetWidth() != lw || m_dirInfo->shadowMap->GetData()->GetHeight() != lh) {
            m_dirInfo->shadowMap->SetData(std::make_shared<Buffer<RGBA>>(lw, lh));
        }

        Camera tmp = scene.camera;
        scene.camera = m_dirInfo->LightCamera;
        FrameBuffer fb{nullptr, m_dirInfo->shadowMap->GetData()};
        Rasterizer::get().pass(scene, RasterizerMode_ZTEST, fb);
        scene.camera = tmp;

        m_dirInfo->LightN = m_dirInfo->LightCamera.get_projection_matrix()
                                            * m_dirInfo->LightCamera.get_view_matrix();
    } else if (m_type == LightType_POINT && m_lightMove) {
        const int size = 512;
        Camera tmp = scene.camera;
        Camera lightCam(m_position + vec3{1, 0, 0}, m_position, {0, 1, 0}, size, size, 90.0, 0.1, 1000.0);
        scene.camera = lightCam;

        double yaws[6] = {0, 180, -90, -90, 90, -90};
        double pitches[6] = {0, 0, 89.9, -89.9, 0, 0};
        FrameBuffer fb;
        for (int i = 0; i < 6; ++i) {
            if (!m_pointInfo->shadowMap->GetData(i) || m_pointInfo->shadowMap->GetData(i)->GetWidth() != size || m_pointInfo->shadowMap->GetData(i)->GetHeight() != size) {
                m_pointInfo->shadowMap->SetData(i, std::make_shared<Buffer<RGBA>>(size, size));
            }
            scene.camera.set_toward_from_center(yaws[i], pitches[i], 0);
            fb = {nullptr, m_pointInfo->shadowMap->GetData(i)};
            Rasterizer::get().pass(scene, RasterizerMode_ZTEST, fb);
            m_pointInfo->LightN[i] = scene.camera.get_projection_matrix()
                                        * scene.camera.get_view_matrix();
        }

        scene.camera = tmp;
    }
    m_lightMove = false;
}

float Light::getVisibility(const vec4& world_pos) const {
    if (!m_haveShadow)
        return 1.0;

    float visibility = 1.0;

    if (m_type == LightType_POINT) {
        vec3 light_dir = normalize(world_pos.to_vec3_point() - m_position);
        int face_index;
        if (std::abs(light_dir.x) >= std::abs(light_dir.y) && std::abs(light_dir.x) >= std::abs(light_dir.z)) {
            face_index = (light_dir.x > 0) ? 0 : 1;
        } else if (std::abs(light_dir.y) >= std::abs(light_dir.x) && std::abs(light_dir.y) >= std::abs(light_dir.z)) {
            face_index = (light_dir.y > 0) ? 2 : 3;
        } else {
            face_index = (light_dir.z > 0) ? 4 : 5;
        }
        vec3 light_space_pos = (m_pointInfo->LightN[face_index] * world_pos).to_vec3_point();
        double z = light_space_pos.z;
        if (ImageUtil::DecodeZ(m_pointInfo->shadowMap->Get(light_dir)) > z + 0.005) {
            visibility = 0.0f; // 在阴影中
        } else {
            visibility = 1.0f; // 可见
        }
    }

    if (m_type == LightType_DIRECTIONAL) {
        vec3 light_space_pos = (m_dirInfo->LightN * world_pos).to_vec3_point();
        double z = light_space_pos.z;
        vec2 uv = {(light_space_pos.x + 1.0) * 0.5,
                       (light_space_pos.y + 1.0) * 0.5};
        if (ImageUtil::DecodeZ(m_dirInfo->shadowMap->Get(uv)) > z + 0.005) {
            visibility = 0.0f; // 在阴影中
        } else {
            visibility = 1.0f; // 可见
        }
    }

    return visibility;
}
