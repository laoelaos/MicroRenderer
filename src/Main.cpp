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

#include "ConfigGui.h"
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

void output_gui();
void glfw_error_callback(int error, const char* description);
void initTexture(int width, int height);
void performRendering();
void loadTextureToGl();

static RenderContext& g_renderContext = ConfigGui::get().getContext();
static Scene g_scene("../obj/default2.sc");
static Rasterizer& g_rasterizer = Rasterizer::get();
static bool saveScene = false;

static GLuint g_renderedTexture = 0;
static std::vector<unsigned char> g_imageData;
static TGAImage g_tgaImage(g_scene.camera.getWidth(), g_scene.camera.getHeight(), TGAImage::RGB);

void MainLoop(GLFWwindow* window) {
    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

    initTexture(g_scene.camera.getWidth(), g_scene.camera.getWidth());
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

        ConfigGui::get().LaunchConfig(g_scene);
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

    if (saveScene)
        g_scene.save_path_file();
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

    auto aspectRatio = static_cast<float>(g_scene.camera.getAspect());

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

    MainLoop(window);

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
        g_scene.camera.rotate_around_point(vec3{0, 0, 0}, g_renderContext.rotation_speed * delta_time * 60.0, 0, 0);
    }

    g_rasterizer.rasterize(g_scene);
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
