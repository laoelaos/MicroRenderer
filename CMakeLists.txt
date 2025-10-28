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
  add_compile_options(-Wall)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
  elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
  endif()
endif()

if(MSVC)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(/O2 /Ot /GL /arch:AVX2)
  elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(/Zi /Od)
  endif()
endif()

find_package(OpenMP COMPONENTS CXX)
find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
add_subdirectory(extern/glfw)

set(IMGUI_SOURCES
        extern/ImGui/imgui.cpp
        extern/ImGui/imgui_stdlib.cpp
        extern/ImGui/imgui_demo.cpp
        extern/ImGui/imgui_draw.cpp
        extern/ImGui/imgui_tables.cpp
        extern/ImGui/imgui_widgets.cpp
        extern/ImGui/imgui_impl_glfw.cpp
        extern/ImGui/imgui_impl_opengl3.cpp
)

set(Graphics_SOURCES
        src/Graphics/Camera.cpp
        src/Graphics/Scene.cpp
        src/Graphics/Light.cpp
        src/Graphics/Model.cpp
        src/Graphics/Geometry.cpp
)

set(GUI_SOURCES
        src/Gui/ConfigGui.cpp
        src/Gui/RenderGui.cpp
        src/Gui/TextureGui.cpp
)

set(UTIL_SOURCES
        src/Util/Buffer.cpp
        src/Util/Logger.cpp
        src/Util/Texture.cpp
        src/Util/ImageUtils.cpp
        src/Util/Path.cpp
)

set(SET_SOURCES
        ${IMGUI_SOURCES}
        ${GUI_SOURCES}
        ${Graphics_SOURCES}
        ${UTIL_SOURCES}
)

set(SOURCES
        ${SET_SOURCES}
        src/main.cpp
        src/Rasterizer.cpp
        src/Shader.cpp
        src/Gui/ConfigGui.cpp
        src/Gui/RenderGui.cpp
        src/Gui/TextureGui.cpp
        src/Gui/TextureGui.h
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        extern/ImGui
        extern/stb
        src/Graphics
        src/Gui
        src/Util
        src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        OpenGL::GL
        $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>
)

# If building with MinGW, copy needed runtime DLLs to bin so Explorer/PowerShell can run directly
if(MINGW)
  get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MINGW_BIN_DIR}/libstdc++-6.dll" "${CMAKE_SOURCE_DIR}/bin/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll" "${CMAKE_SOURCE_DIR}/bin/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MINGW_BIN_DIR}/libwinpthread-1.dll" "${CMAKE_SOURCE_DIR}/bin/"
  )
  if(OpenMP_CXX_FOUND)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MINGW_BIN_DIR}/libgomp-1.dll" "${CMAKE_SOURCE_DIR}/bin/"
    )
  endif()
endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_SOURCE_DIR}/bin/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/obj ${CMAKE_SOURCE_DIR}/bin/obj
)

file(GENERATE OUTPUT .gitignore CONTENT "*")
