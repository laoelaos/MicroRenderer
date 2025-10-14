cmake_minimum_required(VERSION 3.12...3.26)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT (CMAKE_BUILD_TYPE OR DEFINED ENV{CMAKE_BUILD_TYPE}))
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Release default")
endif()

project(MicroRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

option(iwyu "Run include-what-you-use")
if(iwyu)
  find_program(IWYU_EXE NAMES include-what-you-use REQUIRED)
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXE})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|Intel")
  add_compile_options(-Wall -O3)
endif()

if(MSVC)
  add_compile_options(/O2 /Ot /GL)
  add_compile_options(/arch:AVX2)
endif()

find_package(OpenMP COMPONENTS CXX)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

set(IMGUI_SOURCES
        src/ImGui/imgui.cpp
        src/ImGui/imgui_stdlib.cpp
        src/ImGui/imgui_demo.cpp
        src/ImGui/imgui_draw.cpp
        src/ImGui/imgui_tables.cpp
        src/ImGui/imgui_widgets.cpp
        src/ImGui/imgui_impl_glfw.cpp
        src/ImGui/imgui_impl_opengl3.cpp
)

set(Graphics_SOURCES
        src/Graphics/Camera.cpp
        src/Graphics/Scene.cpp
        src/Graphics/Light.cpp
        src/Graphics/Model.cpp
        src/Graphics/Geometry.cpp
)

set(SOURCES ${IMGUI_SOURCES} ${Graphics_SOURCES}
        src/main.cpp
        src/TGAImage.cpp
        src/Rasterizer.cpp
        src/Shader.cpp
        src/ConfigGui.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src/ImGui src/Graphics)

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        OpenGL::GL
        $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>)

file(GENERATE OUTPUT .gitignore CONTENT "*")
