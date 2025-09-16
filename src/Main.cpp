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

// 渲染上下文结构，用于存储和控制渲染参数
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

    // 光照参数
    vec3 light_pos = {20, 20, 20};
    vec3 light_intensity = {2000, 2000, 2000};

    // 材质参数
    phong_properties material_props = {0.9, 0.6, 0.005, 150};

    // 模型路径
    std::string model_path = R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head.obj)";
    std::string diffuse_path = R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head_diffuse.tga)";
    std::string normal_path = R"(D:\CS_learning\Project\MicroRenderer\obj\african_head\african_head_nm.tga)";

    // 渲染选项
    bool diffuse_mapping = true;
    NormalType normal_type = GLOBAL;
    ShadeFrequency shade_frequency = PER_FRAGMENT;

    // 控制标志
    bool should_render = true;
};

static Rasterizer rasterizer(600, 600);
static GLuint renderedTexture = 0;
static std::vector<unsigned char> imageData;
static RenderContext renderContext;
static TGAImage tgaImage(renderContext.width, renderContext.height, TGAImage::RGB);

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void print_utf8_stdout(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    // 格式化为 UTF-8 字节串
    char buffer[4096];
    int n = vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    if (n < 0) return;
#ifdef _WIN32
    // 将 UTF-8 转为 UTF-16 并写入控制台（避免控制台按 OEM/ANSI 错误解码）
    int wlen = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, NULL, 0);
    if (wlen > 0) {
        std::wstring wbuf;
        wbuf.resize(wlen);
        MultiByteToWideChar(CP_UTF8, 0, buffer, -1, &wbuf[0], wlen);
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            // 写入时去掉末尾的 NUL
            WriteConsoleW(h, wbuf.c_str(), static_cast<DWORD>(wbuf.size() - 1), &written, NULL);
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
    rasterizer.load_fragment_shader(std::make_shared<PhongShader>());
    Model model(renderContext.model_path);
    model.material = Material(renderContext.diffuse_path, renderContext.normal_path,
                          renderContext.material_props, renderContext.diffuse_mapping,
                          renderContext.normal_type, renderContext.shade_frequency);
    rasterizer.load_model(model);
    rasterizer.load_lights({{{renderContext.light_pos}, {renderContext.light_intensity}}});
}

void updateTexture() {
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

void performRendering() {
    auto start_time = std::chrono::high_resolution_clock::now();

    if (renderContext.view_change) {
        rasterizer.set_view_matrix(view_matrix(renderContext.eye, renderContext.center, renderContext.up));
        renderContext.view_change = false;
    }
    if (renderContext.proj_change) {
        rasterizer.set_projection_matrix(perspective_projection(renderContext.fov, renderContext.aspect, renderContext.near_, renderContext.far_));
        renderContext.proj_change = false;
    }

    rasterizer.rasterize();
    rasterizer.drawonTGA(tgaImage);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    // 使用跨平台 UTF-8 打印（注意 duration 是整数）
    print_utf8_stdout("渲染完成，耗时: %lld ms\n", static_cast<long long>(duration));
}

void control_gui() {
    ImGui::Begin("渲染控制面板");

        if (ImGui::CollapsingHeader("基本设置", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::InputInt("MSAA级别", &renderContext.msaa_level, 1, 1)) {
                rasterizer.set_options(std::max(1, std::min(4, renderContext.msaa_level)));
            }
        }

        if (ImGui::CollapsingHeader("相机设置", ImGuiTreeNodeFlags_DefaultOpen)) {
            // 临时float变量用于ImGui控件
            float eye_pos[3] = {(float)renderContext.eye.x, (float)renderContext.eye.y, (float)renderContext.eye.z};
            float center_pos[3] = {(float)renderContext.center.x, (float)renderContext.center.y, (float)renderContext.center.z};
            float up_dir[3] = {(float)renderContext.up.x, (float)renderContext.up.y, (float)renderContext.up.z};
            float fov = (float)renderContext.fov;
            float near_plane = (float)renderContext.near_;
            float far_plane = (float)renderContext.far_;

            bool eye_changed = ImGui::InputFloat3("相机位置", eye_pos, "%.2f");
            bool center_changed = ImGui::InputFloat3("观察点", center_pos, "%.2f");
            bool up_changed = ImGui::InputFloat3("上方向", up_dir, "%.2f");
            bool fov_changed = ImGui::InputFloat("视场角FOV", &fov, 1.0f, 5.0f, "%.1f");
            bool near_changed = ImGui::InputFloat("近平面", &near_plane, 0.1f, 0.5f, "%.2f");
            bool far_changed = ImGui::InputFloat("远平面", &far_plane, 0.1f, 0.5f, "%.2f");

            // 添加一些提示信息
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("远平面应大于近平面");

            if (eye_changed) {
                renderContext.eye.x = eye_pos[0];
                renderContext.eye.y = eye_pos[1];
                renderContext.eye.z = eye_pos[2];
                renderContext.view_change = true;
            }

            if (center_changed) {
                renderContext.center.x = center_pos[0];
                renderContext.center.y = center_pos[1];
                renderContext.center.z = center_pos[2];
                renderContext.view_change = true;
            }

            if (up_changed) {
                renderContext.up.x = up_dir[0];
                renderContext.up.y = up_dir[1];
                renderContext.up.z = up_dir[2];
                renderContext.view_change = true;
            }

            if (fov_changed) {
                renderContext.fov = std::max(10.0, std::min(120.0, (double)fov));
                renderContext.proj_change = true;
            }

            if (near_changed) {
                renderContext.near_ = std::max(0.1, (double)near_plane);
                renderContext.proj_change = true;
            }

            if (far_changed) {
                renderContext.far_ = std::max(renderContext.near_ + 0.1, (double)far_plane);
                renderContext.proj_change = true;
            }
        }

        // if (ImGui::CollapsingHeader("光照设置", ImGuiTreeNodeFlags_DefaultOpen)) {
        //     float light_position[3] = {(float)renderContext.light_pos.x, (float)renderContext.light_pos.y, (float)renderContext.light_pos.z};
        //     float light_intens[3] = {(float)renderContext.light_intensity.x, (float)renderContext.light_intensity.y, (float)renderContext.light_intensity.z};
        //
        //     bool light_pos_changed = ImGui::InputFloat3("光源位置", light_position, "%.1f");
        //     bool light_intens_changed = ImGui::InputFloat3("光源强度", light_intens, "%.0f");
        //
        //     if (light_pos_changed) {
        //         renderContext.light_pos.x = light_position[0];
        //         renderContext.light_pos.y = light_position[1];
        //         renderContext.light_pos.z = light_position[2];
        //     }
        //
        //     if (light_intens_changed) {
        //         renderContext.light_intensity.x = light_intens[0];
        //         renderContext.light_intensity.y = light_intens[1];
        //         renderContext.light_intensity.z = light_intens[2];
        //     }
        // }

        // if (ImGui::CollapsingHeader("材质设置", ImGuiTreeNodeFlags_DefaultOpen)) {
        //     float k_ambient = (float)renderContext.material_props.k_ambient;
        //     float k_diffuse = (float)renderContext.material_props.k_diffuse;
        //     float k_specular = (float)renderContext.material_props.k_specular;
        //     int p = renderContext.material_props.p;
        //
        //     bool ambient_changed = ImGui::InputFloat("环境光系数", &k_ambient, 0.05f, 0.1f, "%.2f");
        //     bool diffuse_changed = ImGui::InputFloat("漫反射系数", &k_diffuse, 0.05f, 0.1f, "%.2f");
        //     bool specular_changed = ImGui::InputFloat("镜面反射系数", &k_specular, 0.005f, 0.01f, "%.3f");
        //     bool p_changed = ImGui::InputInt("光泽度", &p, 10, 50);
        //
        //     if (ambient_changed) {
        //         renderContext.material_props.k_ambient = std::max(0.0, std::min(1.0, (double)k_ambient));
        //     }
        //
        //     if (diffuse_changed) {
        //         renderContext.material_props.k_diffuse = std::max(0.0, std::min(1.0, (double)k_diffuse));
        //     }
        //
        //     if (specular_changed) {
        //         renderContext.material_props.k_specular = std::max(0.0, std::min(1.0, (double)k_specular));
        //     }
        //
        //     if (p_changed) {
        //         renderContext.material_props.p = std::max(1, std::min(500, p));
        //     }
        //
        //     const char* normal_types[] = { "全局法线", "切线空间法线" };
        //     static int normal_type_idx = 0;
        //     if (ImGui::Combo("法线类型", &normal_type_idx, normal_types, IM_ARRAYSIZE(normal_types))) {
        //         renderContext.normal_type = normal_type_idx == 0 ? GLOBAL : TANGENT;
        //     }
        //
        //     const char* shade_frequencies[] = { "每顶点着色", "每片段着色" };
        //     static int shade_freq_idx = 1;
        //     if (ImGui::Combo("着色频率", &shade_freq_idx, shade_frequencies, IM_ARRAYSIZE(shade_frequencies))) {
        //         renderContext.shade_frequency = shade_freq_idx == 0 ? PER_VERTEX : PER_FRAGMENT;
        //     }
        //
        //     ImGui::Checkbox("使用贴图", &renderContext.diffuse_mapping);
        // }

        if (ImGui::Button("重新渲染")) {
            renderContext.should_render = true;
        }

        ImGui::End();
}

void output_gui() {
    ImGui::Begin("渲染结果");

    if (renderContext.should_render) {
        print_utf8_stdout("进行重新渲染...\n");
        performRendering();
        updateTexture();
        renderContext.should_render = false;
    }

    // 获取窗口内容区域大小以调整图像大小
    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    // 计算正确的宽高比
    float aspectRatio = static_cast<float>(renderContext.width) / renderContext.height;

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
    ImGui::Text("渲染纹理: ID=%u, 尺寸=%dx%d", renderedTexture,
                tgaImage.width(), tgaImage.height());

    ImGui::Image((void*)(intptr_t)renderedTexture, imageSize);
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
    GLFWwindow *window = glfwCreateWindow((int) (1280 * main_scale), (int) (800 * main_scale),
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

    ImGui::StyleColorsDark(); // Setup Dear ImGui styl     //ImGui::StyleColorsLight();

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
