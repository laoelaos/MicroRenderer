#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cstdio>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <vector>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "Rasterizer.h"
#include "TGAImage.h"
#include "Util.h"
#include <memory>
#include <chrono>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <cstdarg>
#include <string>
#include <thread>

void glfw_error_callback(int error, const char* description);
void print_utf8_stdout(const char* fmt, ...);

void initRasterizer();
void initTexture(int width, int height);

void updateModels();
void updateRotate();
void updateProjection();
void updateLights();

void performRendering();
void loadTextureToGl();

struct ModelInfo {
    std::string name;
    std::string model_path;
    std::string diffuse_path;
    std::string normal_path;
    bool diffuse_mapping = true;
    NormalType normal_type = GLOBAL;
    ShadeFrequency shade_frequency = PER_FRAGMENT;
    phong_properties material_props = {0.9, 0.6, 0.005, 150};
    bool enabled = true;

    // 模型变换参数
    vec3 translation = {0, 0, 0};
    vec3 rotation = {0, 0, 0};  // 欧拉角 (x, y, z) 弧度
    vec3 scale = {1, 1, 1};     // 缩放因子

    mat4 transform = identity_matrix<4>();
};

struct RenderContext {
    // 相机参数
    vec3 eye = {0, 0, 1};
    vec3 center = {0, 0, 2};
    vec3 up = {0, 1, 0};
    double fov = 55.0;
    double aspect = 1.0;
    double near_ = 2.0;
    double far_ = 3.0;

    bool view_change = true;
    bool proj_change = true;

    // 渲染参数
    int width = 600;
    int height = 600;
    int msaa_level = 1;

    std::vector<light> light_sources = {{{20, 20, 20}, {2000, 2000, 2000}}};
    bool lights_changed = true;

    // 模型列表
    std::vector<ModelInfo> models = {
        {
            "head",
            R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head.obj)",
            R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head_diffuse.tga)",
            R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head_nm_tangent.tga)",
            true, TANGENT, PER_FRAGMENT, {0.9, 0.6, 0.005, 150}, true,
            {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, identity_matrix<4>()
        },
        {
            "eye",
            R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head_eye_inner.obj)",
            R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head_eye_inner_diffuse.tga)",
            R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head_eye_inner_nm_tangent.tga)",
            true, TANGENT, PER_FRAGMENT, {0.9, 0.6, 0.005, 150}, true,
            {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, identity_matrix<4>()
        }
    };
    bool models_changed = true;
    int selected_model = 0;

    // 渲染选项
    bool real_time_rendering = false;    // 是否启用实时渲染
    bool auto_rotate = false;           // 是否自动旋转模型
    float rotation_speed = 1.0f;        // 旋转速度
    double current_rotation = 0.0;      // 当前旋转角度

    // 性能监控
    float target_fps = 60.0f;           // 目标帧率
    float current_fps = 0.0f;           // 当前帧率
    long long last_render_time = 0;     // 上次渲染耗时(ms)
    long long refresh_interval = 500;  // 刷新间隔(ms)

    // 控制标志
    bool force_render = true;
};

static Rasterizer rasterizer(600, 600);
static GLuint renderedTexture = 0;
static std::vector<unsigned char> imageData;
static RenderContext renderContext;
static TGAImage tgaImage(renderContext.width, renderContext.height, TGAImage::RGB);

void control_gui() {
    ImGui::Begin("渲染控制面板");

        if (ImGui::CollapsingHeader("基本设置", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::InputInt("MSAA级别", &renderContext.msaa_level, 1, 1)) {
                rasterizer.set_options(std::clamp(renderContext.msaa_level, 1, 10));
            }
        }

        if (ImGui::CollapsingHeader("相机设置", ImGuiTreeNodeFlags_DefaultOpen)) {
            float eye_pos[3] = {static_cast<float>(renderContext.eye.x), static_cast<float>(renderContext.eye.y), static_cast<float>(renderContext.eye.z)};
            float center_pos[3] = {static_cast<float>(renderContext.center.x), static_cast<float>(renderContext.center.y), static_cast<float>(renderContext.center.z)};
            float up_dir[3] = {static_cast<float>(renderContext.up.x), static_cast<float>(renderContext.up.y), static_cast<float>(renderContext.up.z)};
            auto fov = static_cast<float>(renderContext.fov);
            auto near_plane = static_cast<float>(renderContext.near_);
            auto far_plane = static_cast<float>(renderContext.far_);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("远平面应大于近平面");

            if (ImGui::InputFloat3("相机位置", eye_pos, "%.2f")) {
                renderContext.eye.x = eye_pos[0];
                renderContext.eye.y = eye_pos[1];
                renderContext.eye.z = eye_pos[2];
                renderContext.view_change = true;
            }
            if (ImGui::InputFloat3("观察点", center_pos, "%.2f")) {
                renderContext.center.x = center_pos[0];
                renderContext.center.y = center_pos[1];
                renderContext.center.z = center_pos[2];
                renderContext.view_change = true;
            }
            if (ImGui::InputFloat3("上方向", up_dir, "%.2f")) {
                renderContext.up.x = up_dir[0];
                renderContext.up.y = up_dir[1];
                renderContext.up.z = up_dir[2];
                renderContext.view_change = true;
            }
            if (ImGui::InputFloat("视场角FOV", &fov, 1.0f, 5.0f, "%.1f")) {
                renderContext.fov = std::clamp(static_cast<double>(fov), 10.0, 120.0);
                renderContext.proj_change = true;
            }
            if (ImGui::InputFloat("近平面", &near_plane, 0.1f, 0.5f, "%.2f")) {
                renderContext.near_ = std::max(0.1, static_cast<double>(near_plane));
                renderContext.proj_change = true;
            }
            if (ImGui::InputFloat("远平面", &far_plane, 0.1f, 0.5f, "%.2f")) {
                renderContext.far_ = std::max(renderContext.near_ + 0.1, static_cast<double>(far_plane));
                renderContext.proj_change = true;
            }
        }

        if (ImGui::CollapsingHeader("光照设置", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("光源数量: %zu", renderContext.light_sources.size());

            if (ImGui::Button("添加光源")) {
                renderContext.light_sources.push_back({{20, 20, 20}, {2000, 2000, 2000}});
                renderContext.lights_changed = true;
            }
            
            ImGui::Separator();

            // 显示每个光源的设置
            for (size_t i = 0; i < renderContext.light_sources.size(); i++) {
                ImGui::PushID(static_cast<int>(i));
                
                std::string header = "光源 " + std::to_string(i + 1);
                if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    float light_position[3] = {
                        static_cast<float>(renderContext.light_sources[i].position.x),
                        static_cast<float>(renderContext.light_sources[i].position.y),
                        static_cast<float>(renderContext.light_sources[i].position.z)
                    };
                    float light_intensity[3] = {
                        static_cast<float>(renderContext.light_sources[i].intensity.x),
                        static_cast<float>(renderContext.light_sources[i].intensity.y),
                        static_cast<float>(renderContext.light_sources[i].intensity.z)
                    };

                    if (ImGui::InputFloat3("位置", light_position, "%.1f")) {
                        renderContext.light_sources[i].position = {light_position[0], light_position[1], light_position[2]};
                        renderContext.lights_changed = true;
                    }
                    
                    if (ImGui::InputFloat3("强度", light_intensity, "%.0f")) {
                        renderContext.light_sources[i].intensity = {light_intensity[0], light_intensity[1], light_intensity[2]};
                        renderContext.lights_changed = true;
                    }
                    
                    // 删除光源按钮
                    if (renderContext.light_sources.size() > 1) {  // 至少保留一个光源
                        ImGui::SameLine();
                        if (ImGui::Button("删除")) {
                            renderContext.light_sources.erase(renderContext.light_sources.begin() + static_cast<int64_t>(i));
                            renderContext.lights_changed = true;
                            ImGui::PopID();
                            break;  // 退出循环，因为vector已经改变
                        }
                    }
                }
                ImGui::PopID();
            }
        }

        if (ImGui::CollapsingHeader("模型管理", ImGuiTreeNodeFlags_DefaultOpen)) {
            // 显示模型列表
            ImGui::Text("场景中的模型: %zu", renderContext.models.size());

            if (ImGui::Button("添加新模型")) {
                // 添加默认模型
                renderContext.models.push_back({
                    "newObj_" + std::to_string(renderContext.models.size()),
                    R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head.obj)",
                    R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head_diffuse.tga)",
                    R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head_nm.tga)",
                    true, GLOBAL, PER_FRAGMENT, {0.9, 0.6, 0.005, 150}, true,
                    {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, identity_matrix<4>()
                });
                renderContext.models_changed = true;
            }

            ImGui::Separator();

            // 模型选择列表
            static std::vector<const char*> model_names;
            model_names.clear();
            for (size_t i = 0; i < renderContext.models.size(); i++) {
                model_names.push_back(renderContext.models[i].name.c_str());
            }

            // 模型选择下拉菜单
            if (!model_names.empty()) {
                if (ImGui::Combo("选择模型", &renderContext.selected_model, model_names.data(), static_cast<int>(model_names.size()))) {
                    // 模型选择改变
                }

                // 如果有选择的模型，显示编辑界面
                if (renderContext.selected_model >= 0 && renderContext.selected_model < static_cast<int>(renderContext.models.size())) {
                    ModelInfo &model = renderContext.models[renderContext.selected_model];

                    if (ImGui::TreeNode("模型基础设置")) {
                        if (ImGui::Checkbox("启用模型", &model.enabled)) {
                            renderContext.models_changed = true;
                        }

                        char name[256];
                        strncpy(name, model.name.c_str(), sizeof(name) - 1);
                        name[sizeof(name) - 1] = '\0';
                        if (ImGui::InputText("模型名称", name, sizeof(name))) {
                            model.name = name;
                        }

                        strncpy(name, model.model_path.c_str(), sizeof(name) - 1);
                        name[sizeof(name) - 1] = '\0';
                        if (ImGui::InputText("模型文件路径", name, sizeof(name))) {
                            model.model_path = name;
                            renderContext.models_changed = true;
                        }

                        strncpy(name, model.diffuse_path.c_str(), sizeof(name) - 1);
                        name[sizeof(name) - 1] = '\0';
                        if (ImGui::InputText("漫反射贴图路径", name, sizeof(name))) {
                            model.diffuse_path = name;
                            renderContext.models_changed = true;
                        }

                        strncpy(name, model.normal_path.c_str(), sizeof(name) - 1);
                        name[sizeof(name) - 1] = '\0';
                        if (ImGui::InputText("法线贴图路径", name, sizeof(name))) {
                            model.normal_path = name;
                            renderContext.models_changed = true;
                        }

                        // 材质属性
                        auto k_ambient = static_cast<float>(model.material_props.k_ambient);
                        auto k_diffuse = static_cast<float>(model.material_props.k_diffuse);
                        auto k_specular = static_cast<float>(model.material_props.k_specular);
                        int p = model.material_props.p;

                        if (ImGui::InputFloat("环境光系数", &k_ambient, 0.05f, 0.1f, "%.2f")) {
                            model.material_props.k_ambient = std::clamp(static_cast<double>(k_ambient), 0.0, 1.0);
                            renderContext.models_changed = true;
                        }
                        if (ImGui::InputFloat("漫反射系数", &k_diffuse, 0.05f, 0.1f, "%.2f")) {
                            model.material_props.k_diffuse = std::clamp(static_cast<double>(k_diffuse), 0.0, 1.0);
                            renderContext.models_changed = true;
                        }
                        if (ImGui::InputFloat("镜面反射系数", &k_specular, 0.005f, 0.01f, "%.3f")) {
                            model.material_props.k_specular = std::clamp(static_cast<double>(k_specular), 0.0, 1.0);
                            renderContext.models_changed = true;
                        }
                        if (ImGui::InputInt("光泽度", &p, 10, 50)) {
                            model.material_props.p = std::clamp(p, 1, 1000);
                            renderContext.models_changed = true;
                        }

                        // 贴图选项
                        if (ImGui::Checkbox("使用贴图", &model.diffuse_mapping)) {
                            renderContext.models_changed = true;
                        }

                        // 法线类型
                        const char *normal_types[] = {"全局法线贴图", "切线空间法线贴图"};
                        int normal_type_idx = model.normal_type == GLOBAL ? 0 : 1;
                        if (ImGui::Combo("法线类型", &normal_type_idx, normal_types, IM_ARRAYSIZE(normal_types))) {
                            model.normal_type = normal_type_idx == 0 ? GLOBAL : TANGENT;
                            renderContext.models_changed = true;
                        }

                        // 着色频率
                        const char *shade_frequencies[] = {"每顶点着色", "每片段着色"};
                        int shade_freq_idx = model.shade_frequency == PER_VERTEX ? 0 : 1;
                        if (ImGui::Combo("着色频率", &shade_freq_idx, shade_frequencies, IM_ARRAYSIZE(shade_frequencies))) {
                            model.shade_frequency = shade_freq_idx == 0 ? PER_VERTEX : PER_FRAGMENT;
                            renderContext.models_changed = true;
                        }

                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("模型变换")) {
                        // 位移控制
                        float translation[3] = {
                            static_cast<float>(model.translation.x),
                            static_cast<float>(model.translation.y),
                            static_cast<float>(model.translation.z)
                        };
                        if (ImGui::InputFloat3("位移", translation, "%.2f")) {
                            model.translation = {translation[0], translation[1], translation[2]};
                            model.transform = calculate_transform_matrix(model.translation, model.rotation, model.scale);
                            renderContext.models_changed = true;
                        }

                        // 旋转控制 - 使用滑条 (角度制显示)
                        float rotation_degrees[3] = {
                            static_cast<float>(model.rotation.x * 180.0 / M_PI),
                            static_cast<float>(model.rotation.y * 180.0 / M_PI),
                            static_cast<float>(model.rotation.z * 180.0 / M_PI)
                        };

                        if (ImGui::SliderFloat("X轴旋转", &rotation_degrees[0], -180.0f, 180.0f, "%.1f°")) {
                            model.rotation.x = rotation_degrees[0] * M_PI / 180.0;
                            model.transform = calculate_transform_matrix(model.translation, model.rotation, model.scale);
                            renderContext.models_changed = true;
                        }

                        if (ImGui::SliderFloat("Y轴旋转", &rotation_degrees[1], -180.0f, 180.0f, "%.1f°")) {
                            model.rotation.y = rotation_degrees[1] * M_PI / 180.0;
                            model.transform = calculate_transform_matrix(model.translation, model.rotation, model.scale);
                            renderContext.models_changed = true;
                        }

                        if (ImGui::SliderFloat("Z轴旋转", &rotation_degrees[2], -180.0f, 180.0f, "%.1f°")) {
                            model.rotation.z = rotation_degrees[2] * M_PI / 180.0;
                            model.transform = calculate_transform_matrix(model.translation, model.rotation, model.scale);
                            renderContext.models_changed = true;
                        }

                        // 缩放控制 - 使用输入框
                        float scale[3] = {
                            static_cast<float>(model.scale.x),
                            static_cast<float>(model.scale.y),
                            static_cast<float>(model.scale.z)
                        };

                        if (ImGui::InputFloat3("缩放", scale, "%.2f")) {
                            // 限制缩放不能为零或负数
                            model.scale = {
                                std::max(0.01, static_cast<double>(scale[0])),
                                std::max(0.01, static_cast<double>(scale[1])),
                                std::max(0.01, static_cast<double>(scale[2]))
                            };
                            model.transform = calculate_transform_matrix(model.translation, model.rotation, model.scale);
                            renderContext.models_changed = true;
                        }

                        // 统一缩放按钮和滑条
                        ImGui::Separator();

                        static float uniform_scale = 1.0f;
                        if (ImGui::SliderFloat("统一缩放", &uniform_scale, 0.1f, 5.0f, "%.2f")) {
                            // 不更新model.scale，仅供操作
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("应用统一缩放")) {
                            model.scale = {uniform_scale, uniform_scale, uniform_scale};
                            model.transform = calculate_transform_matrix(model.translation, model.rotation, model.scale);
                            renderContext.models_changed = true;
                        }

                        // 重置变换按钮
                        if (ImGui::Button("重置所有变换")) {
                            model.translation = {0, 0, 0};
                            model.rotation = {0, 0, 0};
                            model.scale = {1, 1, 1};
                            model.transform = identity_matrix<4>();
                            renderContext.models_changed = true;
                            uniform_scale = 1.0f;
                        }

                        ImGui::TreePop();
                    }

                    // 删除模型按钮
                    ImGui::Separator();
                    if (renderContext.models.size() > 1) {  // 至少保留一个模型
                        if (ImGui::Button("删除模型")) {
                            renderContext.models.erase(renderContext.models.begin() + renderContext.selected_model);
                            renderContext.selected_model = std::min(renderContext.selected_model, static_cast<int>(renderContext.models.size()) - 1);
                            renderContext.models_changed = true;
                        }
                    }
                }
            }
        }

        if (ImGui::CollapsingHeader("实时渲染设置", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("启用实时渲染", &renderContext.real_time_rendering);
            ImGui::Checkbox("自动旋转模型", &renderContext.auto_rotate);

            if (renderContext.auto_rotate) {
                ImGui::SliderFloat("旋转速度", &renderContext.rotation_speed, 0.1f, 5.0f, "%.1f");
            }

            // 目标帧率设置
            ImGui::SliderFloat("目标帧率", &renderContext.target_fps, 10.0f, 120.0f, "%.1f");

            int refresh_interval = static_cast<int>(renderContext.refresh_interval);
            ImGui::InputInt("信息刷新间隔(ms)", &refresh_interval);
            renderContext.refresh_interval = std::max(100, refresh_interval);
        }

        if (ImGui::Button("强制重新渲染")) {
            renderContext.force_render = true;
        }

        ImGui::End();
}

void output_gui() {
    ImGui::Begin("渲染结果");

    // 计算渲染所需时间，不包括跳过渲染只进行imgui的时间
    static auto last_frame_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    auto frame_duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_frame_time).count();

    if (renderContext.force_render) {
        performRendering();
        loadTextureToGl();
        renderContext.force_render = false;
        renderContext.current_fps = 0.0f;
    }

    static int frame_count = 0;
    static long long frame_time_accumulator = 0;
    if (renderContext.real_time_rendering ||
        renderContext.auto_rotate) {
        frame_count++;
        frame_time_accumulator += frame_duration;

        performRendering();
        loadTextureToGl();
        renderContext.force_render = false;
        last_frame_time = current_time;
        if (frame_duration > 0 && frame_time_accumulator > renderContext.refresh_interval) {
            renderContext.current_fps = 1000.0f / static_cast<float>(frame_time_accumulator) * frame_count;
            frame_count = 0;
            frame_time_accumulator = 0;
        }
    }

    // 获取窗口内容区域大小以调整���像大小
    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    // 计算正确的宽高比
    float aspectRatio = static_cast<float>(renderContext.width) / static_cast<float>(renderContext.height);

    // ���示纹理，保持纵横比
    ImVec2 imageSize;
    if (windowSize.x / aspectRatio <= windowSize.y) {
        imageSize = ImVec2(windowSize.x, windowSize.x / aspectRatio);
    } else {
        imageSize = ImVec2(windowSize.y * aspectRatio, windowSize.y);
    }

    // 居中显示图像
    float offsetX = (windowSize.x - imageSize.x) * 0.5f;
    float offsetY = (windowSize.y - imageSize.y) * 0.5f;
    if (offsetX > 0 || offsetY > 0) {
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY));
    }

    // 显示纹理信息和预览
    ImGui::Text("渲染纹理: ID=%u, 尺寸=%dx%d", renderedTexture,
                tgaImage.width(), tgaImage.height());
    ImGui::Text("渲染时间: %lld ms | FPS: %.1f", renderContext.last_render_time, renderContext.current_fps);

    // 显示渲染模式状态
    if (renderContext.real_time_rendering) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "实时渲染已启用");
    } else {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "按需渲染模式");
    }

    ImGui::Image((void*)static_cast<intptr_t>(renderedTexture), imageSize);
    ImGui::End();
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

#if defined(IMGUI_IMPL_OPENGL_ES2)  // Decide GL+GLSL versions
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow *window = glfwCreateWindow(static_cast<int>(1280 * main_scale), static_cast<int>(800 * main_scale),
                                          "MicroRenderer", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION(); // Setup Dear ImGui context
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    ImGui::StyleColorsDark(); // Setup Dear ImGui style

    ImGuiStyle &style = ImGui::GetStyle(); // Setup scaling
    style.ScaleAllSizes(main_scale);
    // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;
    // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    ImGui_ImplGlfw_InitForOpenGL(window, true); // Setup Platform/Renderer backends
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImFont *font = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\msyh.ttc)", 18.0f);
    IM_ASSERT(font != nullptr);

    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

    initTexture(renderContext.width, renderContext.height);
    initRasterizer();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();   // Start the Dear ImGui frame
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        control_gui();
        output_gui();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 清理资源
    if (renderedTexture != 0) {
        glDeleteTextures(1, &renderedTexture);
    }

    ImGui_ImplOpenGL3_Shutdown();   // Cleanup
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void print_utf8_stdout(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    // 格式化为 UTF-8 字节串
    char buffer[4096];
    int n = vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    if (n < 0) return;
#ifdef _WIN32
    // 将 UTF-8 转为 UTF-16 并写入控制台（避免控制台按 OEM/ANSI 错误解码）
    int wlen = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, nullptr, 0);
    if (wlen > 0) {
        std::wstring wbuf;
        wbuf.resize(wlen);
        MultiByteToWideChar(CP_UTF8, 0, buffer, -1, &wbuf[0], wlen);
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            // 写入时去掉末尾的 NUL
            WriteConsoleW(h, wbuf.c_str(), static_cast<DWORD>(wbuf.size() - 1), &written, nullptr);
        }
    }
#else
    // 非 Windows，标准终端一般能正确显示 UTF-8
    fputs(buffer, stdout);
#endif
}

void initTexture(int width, int height) {
    if (renderedTexture != 0) {
        glDeleteTextures(1, &renderedTexture);
    }

    glGenTextures(1, &renderedTexture);
    glBindTexture(GL_TEXTURE_2D, renderedTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);

    // 初始化一个空白纹理
    imageData.resize(width * height * 4, 255); // 全白RGBA格式
}

void initRasterizer() {
    rasterizer.set_model_matrix(model_matrix());
    rasterizer.set_view_matrix(view_matrix(renderContext.eye, renderContext.center, renderContext.up));
    rasterizer.set_projection_matrix(perspective_projection(renderContext.fov, renderContext.aspect, renderContext.near_, renderContext.far_));
    rasterizer.load_fragment_shader(std::make_shared<PhongShader>());
    updateModels();
    updateLights();
}

void updateModels() {
    if (!renderContext.models_changed)
        return;
    
    rasterizer.clear_models();
    for (const auto& modelInfo : renderContext.models) {
        if (modelInfo.enabled) {
            try {
                Model model(modelInfo.model_path);
                model.material = Material(modelInfo.diffuse_path, modelInfo.normal_path,
                                     modelInfo.material_props, modelInfo.diffuse_mapping,
                                     modelInfo.normal_type, modelInfo.shade_frequency);
                model.transform = modelInfo.transform;
                rasterizer.load_model(model);
            }
            catch (const std::exception& e) {
                print_utf8_stdout("加载模型失败: %s - %s\n", modelInfo.model_path.c_str(), e.what());
            }
        }
    }
    renderContext.models_changed = false;
}

void updateRotate() {
    if (!renderContext.auto_rotate)
        return;

    double delta_time = 1.0 / renderContext.target_fps; // 估算的时间间隔
    renderContext.current_rotation += renderContext.rotation_speed * delta_time * 60.0; // 旋转速度调整
    if (renderContext.current_rotation > 360.0) {
        renderContext.current_rotation -= 360.0;
    }

    // 创建旋转矩阵并设置到光栅化器
    mat4 rotation_matrix = {{
        {cos(renderContext.current_rotation * M_PI / 180.0), 0, sin(renderContext.current_rotation * M_PI / 180.0), 0},
        {0, 1, 0, 0},
        {-sin(renderContext.current_rotation * M_PI / 180.0), 0, cos(renderContext.current_rotation * M_PI / 180.0), 0},
        {0, 0, 0, 1}
    }};
    rasterizer.set_view_matrix(view_matrix(renderContext.eye, renderContext.center, renderContext.up) * rotation_matrix);
}

void updateProjection() {
    if (renderContext.view_change) {
        rasterizer.set_view_matrix(view_matrix(renderContext.eye, renderContext.center, renderContext.up));
        renderContext.view_change = false;
    }
    if (renderContext.proj_change) {
        rasterizer.set_projection_matrix(perspective_projection(renderContext.fov, renderContext.aspect, renderContext.near_, renderContext.far_));
        renderContext.proj_change = false;
    }
}

void updateLights() {
    if (renderContext.lights_changed) {
        rasterizer.clear_lights();
        rasterizer.load_lights(renderContext.light_sources);
        renderContext.lights_changed = false;
    }
}

void performRendering() {
    auto start_time = std::chrono::high_resolution_clock::now();

    updateModels();
    updateRotate();
    updateProjection();
    updateLights();

    rasterizer.clear_buffer();
    rasterizer.rasterize();
    rasterizer.drawonTGA(tgaImage);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    static long long frame_count = 0;
    frame_count += duration;
    if (frame_count >= renderContext.refresh_interval) {
        renderContext.last_render_time = duration;
        frame_count = 0;
    }
}

void loadTextureToGl() {
    int width = tgaImage.width();
    int height = tgaImage.height();

    // 从TGA转换到RGBA格式
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            TGAColor color = tgaImage.get(x, y);
            int index = ((height - y - 1) * width + x) * 4;
            imageData[index] = color.bgra[2];     // R
            imageData[index + 1] = color.bgra[1]; // G
            imageData[index + 2] = color.bgra[0]; // B
            imageData[index + 3] = 255;           // A (强制设为不透明)
        }
    }

    glBindTexture(GL_TEXTURE_2D, renderedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
}
