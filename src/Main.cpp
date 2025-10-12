#include "imgui.h"
#include "imgui_stdlib.h"
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

#include "Scene.h"

void glfw_error_callback(int error, const char* description);
void initTexture(int width, int height);
void performRendering();
void loadTextureToGl();

struct RenderContext {
    int msaa_level = 1;

    bool use_euler_angles = false;

    int selected_model = 0;

    // 渲染选项
    bool real_time_rendering = true;    // 是否启用实时渲染
    bool auto_rotate = false;           // 是否自动旋转模型
    float rotation_speed = 1.0f;        // 旋转速度
    double current_rotation = 0.0;      // 当前旋转角度

    // 性能监控
    float target_fps = 60.0f;           // 目标帧率
    float current_fps = 0.0f;           // 当前帧率
    long long avg_render_time = 0;     // 上次渲染耗时(ms)
    long long refresh_interval = 500;  // 刷新间隔(ms)

    // 控制标志
    bool force_render = true;
};

static RenderContext g_renderContext;
static Scene g_scene("../obj/default2.sc");
static Rasterizer g_rasterizer = {};

static GLuint g_renderedTexture = 0;
static std::vector<unsigned char> g_imageData;
static TGAImage g_tgaImage(g_scene.camera.width, g_scene.camera.height, TGAImage::RGB);

void control_gui() {
    ImGui::Begin("渲染控制面板");

    //TODO: 渲染类型选择
    if (ImGui::CollapsingHeader("基本设置")) {
        if (ImGui::InputInt("MSAA级别", &g_renderContext.msaa_level, 1, 1)) {
            g_rasterizer.set_msaa(std::clamp(g_renderContext.msaa_level, 1, 10));
        }
    }

    if (ImGui::CollapsingHeader("相机设置")) {
        std::array<float, 3> eye_pos = to_float_array(g_scene.camera.eye);
        std::array<float, 3> center_pos = to_float_array(g_scene.camera.center);
        std::array<float, 3> up_dir = to_float_array(g_scene.camera.up);
        auto fov = static_cast<float>(g_scene.camera.fov);
        auto near_plane = static_cast<float>(g_scene.camera.near_);
        auto far_plane = static_cast<float>(g_scene.camera.far_);
        auto yaw = static_cast<float>(g_scene.camera.yaw);
        auto pitch = static_cast<float>(g_scene.camera.pitch);
        auto roll = static_cast<float>(g_scene.camera.roll);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("远平面应大于近平面");

        ImGui::Checkbox("使用滑条控制相机欧拉角", &g_renderContext.use_euler_angles);

        if (g_renderContext.use_euler_angles) {
            bool view_changed = false;
            if (ImGui::SliderFloat("水平旋转 (Yaw)", &yaw, -180.0f, 180.0f, "%.1f°"))
                view_changed = true;
            if (ImGui::SliderFloat("垂直旋转 (Pitch)", &pitch, -89.0f, 89.0f, "%.1f°"))
                view_changed = true;
            if (ImGui::SliderFloat("滚转 (roll)", &roll, -180.0f, 180.0f, "%.1f°"))
                view_changed = true;
            if (view_changed)
                g_scene.camera.set_toward_from_center(yaw, pitch, roll);

            if (ImGui::InputFloat3("相机位置", eye_pos.data(), "%.2f"))
                g_scene.camera.eye = float_array_to_vec(eye_pos);
            ImGui::BeginDisabled();
            ImGui::InputFloat3("上方向", to_float_array(g_scene.camera.up).data(), "%.2f");
            ImGui::EndDisabled();
        } else {
            if (ImGui::InputFloat3("观察点", eye_pos.data(), "%.2f"))
                g_scene.camera.eye = float_array_to_vec(eye_pos);
            if (ImGui::InputFloat3("相机位置", center_pos.data(), "%.2f"))
                g_scene.camera.center = float_array_to_vec(center_pos);
            if (ImGui::InputFloat3("上方向", up_dir.data(), "%.2f"))
                g_scene.camera.up = float_array_to_vec(up_dir);
        }

        if (ImGui::InputFloat("视场角FOV", &fov, 1.0f, 5.0f, "%.1f"))
            g_scene.camera.fov = fov;
        if (ImGui::InputFloat("近平面", &near_plane, 0.1f, 0.5f, "%.2f"))
            g_scene.camera.near_ = near_plane;
        if (ImGui::InputFloat("远平面", &far_plane, 0.1f, 0.5f, "%.2f"))
            g_scene.camera.far_ = far_plane;

        if (ImGui::Button("重置相机"))
            g_scene.camera = {};
    }

    //TODO: LightCamera管理
    if (ImGui::CollapsingHeader("光照设置")) {
        ImGui::Text("光源数量: %zu", g_scene.lights.size());

        if (ImGui::Button("添加光源"))
            g_scene.lights.emplace_back();

        ImGui::Separator();
        for (size_t i = 0; i < g_scene.lights.size(); i++) {
            ImGui::PushID(static_cast<int>(i));

            std::string header = "光源 " + std::to_string(i + 1);
            if (ImGui::TreeNode(header.c_str())) {
                std::array<float, 3> light_position = to_float_array(g_scene.lights[i].position);
                std::array<float, 3> light_color = to_float_array(g_scene.lights[i].color);
                auto light_intensity = static_cast<float>(g_scene.lights[i].intensity);

                if (ImGui::InputFloat3("位置", light_position.data(), "%.1f")) {
                    g_scene.lights[i].setPosition(float_array_to_vec(light_position));
                    if (g_scene.lights[i].getType() == DIRECTIONAL_LIGHT) {
                        g_scene.lights[i].lightMove = true;
                    }
                }
                if (ImGui::ColorEdit3("颜色", light_color.data()))
                    g_scene.lights[i].color = {light_color[0], light_color[1], light_color[2]};
                if (ImGui::InputFloat("光强", &light_intensity, 10.0f, 100.0f, "%.1f"))
                    g_scene.lights[i].intensity = std::max(0.0f, light_intensity);

                if (g_scene.lights.size() > 1) {
                    ImGui::SameLine();
                    if (ImGui::Button("删除")) {
                        g_scene.lights.erase(g_scene.lights.begin() + static_cast<int64_t>(i));
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
        ImGui::Text("场景中的模型个数: %zu", g_scene.models.size());

        if (ImGui::Button("添加新模型"))
            g_scene.models.emplace_back();

        ImGui::Separator();

        std::vector<const char*> model_names;
        for (auto & model : g_scene.models)
            model_names.push_back(model.name.c_str());

        if (!model_names.empty()) {
            ImGui::Combo("选择模型", &g_renderContext.selected_model, model_names.data(), static_cast<int>(model_names.size()));

            if (g_renderContext.selected_model >= 0 && g_renderContext.selected_model < static_cast<int>(g_scene.models.size())) {
                Model& model = g_scene.models[g_renderContext.selected_model];

                if (ImGui::TreeNode("模型基础设置")) {
                    ImGui::Checkbox("启用模型", &model.enable);

                    ImGui::InputText("模型名称", &model.name);
                    if (ImGui::InputText("模型文件路径", &model.name))
                        model.load_obj();
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
                    std::array<float, 3> translation = to_float_array(model.translation);
                    if (ImGui::InputFloat3("位移", translation.data(), "%.2f"))
                        model.translation = float_array_to_vec(translation);

                    std::array<float, 3> rotation_degrees = to_float_array(model.rotation); //角度制
                    if (ImGui::SliderFloat("X轴旋转", &rotation_degrees[0], -180.0f, 180.0f, "%.1f°"))
                        model.rotation = float_array_to_vec(rotation_degrees);
                    if (ImGui::SliderFloat("Y轴旋转", &rotation_degrees[1], -180.0f, 180.0f, "%.1f°"))
                        model.rotation = float_array_to_vec(rotation_degrees);
                    if (ImGui::SliderFloat("Z轴旋转", &rotation_degrees[2], -180.0f, 180.0f, "%.1f°"))
                        model.rotation = float_array_to_vec(rotation_degrees);

                    std::array<float, 3> scale = to_float_array(model.scale);
                    if (ImGui::InputFloat3("缩放", scale.data(), "%.2f"))
                        model.scale = {
                        std::max(0.01, static_cast<double>(scale[0])),
                        std::max(0.01, static_cast<double>(scale[1])),
                        std::max(0.01, static_cast<double>(scale[2]))
                    };

                    ImGui::Separator();

                    static float uniform_scale = 1.0f;
                    ImGui::SliderFloat("统一缩放", &uniform_scale, 0.1f, 5.0f, "%.2f");
                    ImGui::SameLine();
                    if (ImGui::Button("应用统一缩放"))
                        model.scale = {uniform_scale, uniform_scale, uniform_scale};

                    if (ImGui::Button("重置所有变换")) {
                        model.translation = {0, 0, 0};
                        model.rotation = {0, 0, 0};
                        model.scale = {1, 1, 1};
                        uniform_scale = 1.0f;
                    }

                    ImGui::TreePop();
                }

                ImGui::Separator();
                if (g_scene.models.size() > 1) {  // 至少保留一个模型
                    if (ImGui::Button("删除模型")) {
                        g_scene.models.erase(g_scene.models.begin() + g_renderContext.selected_model);
                        g_renderContext.selected_model = std::min(g_renderContext.selected_model, static_cast<int>(g_scene.models.size()) - 1);
                    }
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("实时渲染设置", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("启用实时渲染", &g_renderContext.real_time_rendering);
            ImGui::Checkbox("自动旋转模型", &g_renderContext.auto_rotate);

            if (g_renderContext.auto_rotate) {
                ImGui::SliderFloat("旋转速度", &g_renderContext.rotation_speed, 0.1f, 5.0f, "%.1f");
            }

            // 目标帧率设置
            ImGui::SliderFloat("目标帧率", &g_renderContext.target_fps, 10.0f, 120.0f, "%.1f");

            int refresh_interval = static_cast<int>(g_renderContext.refresh_interval);
            ImGui::InputInt("信息刷新间隔(ms)", &refresh_interval);
            g_renderContext.refresh_interval = std::max(100, refresh_interval);
        }

    if (ImGui::Button("强制重新渲染")) {
        g_renderContext.force_render = true;
    }

    ImGui::End();
}

void output_gui() {
    ImGui::Begin("渲染结果");

    if (g_renderContext.force_render ||
        g_renderContext.real_time_rendering ||
        g_renderContext.auto_rotate) {

        performRendering();
        loadTextureToGl();
        g_renderContext.force_render = false;
    }

    if (g_renderContext.force_render) {
        g_renderContext.current_fps = 0.0f;
    }

    // 获取窗口内容区域大小以调整图像大小
    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    auto aspectRatio = static_cast<float>(g_scene.camera.aspect);

    // 显示纹理，保持纵横比
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
    ImGui::Text("渲染纹理: ID=%u, 尺寸=%dx%d", g_renderedTexture,
                g_tgaImage.width(), g_tgaImage.height());
    ImGui::Text("渲染时间: %lld ms | FPS: %.1f", g_renderContext.avg_render_time, g_renderContext.current_fps);

    // 显示渲染模式状态
    if (g_renderContext.real_time_rendering || g_renderContext.auto_rotate) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "实时渲染已启用");
    } else {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "按需渲染模式");
    }

    ImGui::Image((void*)static_cast<intptr_t>(g_renderedTexture), imageSize);
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

    initTexture(g_scene.camera.width, g_scene.camera.height);
    g_rasterizer.set_msaa(1);
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

    //scene.save_path_file();

    // 清理资源
    if (g_renderedTexture != 0) {
        glDeleteTextures(1, &g_renderedTexture);
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

void initTexture(int width, int height) {
    if (g_renderedTexture != 0) {
        glDeleteTextures(1, &g_renderedTexture);
    }

    glGenTextures(1, &g_renderedTexture);
    glBindTexture(GL_TEXTURE_2D, g_renderedTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);

    // 初始化一个空白纹理
    g_imageData.resize(width * height * 4, 255); // 全白RGBA格式
}

void performRendering() {
    auto start_time = std::chrono::high_resolution_clock::now();


    if (g_renderContext.auto_rotate) {
        double delta_time = 1.0 / g_renderContext.target_fps; // 估算的时间间隔
        g_scene.camera.rotate_around_eye(g_renderContext.rotation_speed * delta_time * 60.0, 0, 0);
    }

    g_rasterizer.rasterize(g_scene, PHONG_WITH_SHADOW);
    g_rasterizer.framebuffer_to_TGA(g_tgaImage);


    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    static int frame_counter = 0;
    static long long duration_counter = 0;
    frame_counter++;
    duration_counter += duration;
    if (duration_counter >= g_renderContext.refresh_interval) {
        g_renderContext.avg_render_time = duration_counter / frame_counter;
        g_renderContext.current_fps = 1000.0f / static_cast<float>(g_renderContext.avg_render_time);
        frame_counter = 0;
        duration_counter = 0;
    }
}

void loadTextureToGl() {
    int width = g_tgaImage.width();
    int height = g_tgaImage.height();

    // 从TGA转换到RGBA格式
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            TGAColor color = g_tgaImage.get(x, y);
            int index = ((height - y - 1) * width + x) * 4;
            g_imageData[index] = color.bgra[2];     // R
            g_imageData[index + 1] = color.bgra[1]; // G
            g_imageData[index + 2] = color.bgra[0]; // B
            g_imageData[index + 3] = 255;           // A (强制设为不透明)
        }
    }

    glBindTexture(GL_TEXTURE_2D, g_renderedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, g_imageData.data());
}
