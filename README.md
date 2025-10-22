# MicroRenderer

一个用 C++20 构建的软光栅化渲染器，从零开始实现了完整的 3D 渲染管线。

![主界面截图](/obj/ss1.jpg)
*<p align="center">主界面：包含配置面板、渲染视图和纹理查看器。</p>*

## 主要特性

- **核心渲染**:
  - 基于 CPU 的分块光栅化，支持多线程（OpenMP）
  - 多重采样抗锯齿 (MSAA)
  - Z-Buffer 深度测试
- **光照与着色**:
  - Phong 光照模型（环境光、漫反射、镜面反射）
  - Shadow Mapping 阴影
  - 法线贴图（切线空间/模型空间）
- **高级功能**:
  - 天空盒（球形/立方体贴图）
  - 灵活的纹理系统（多种环绕和过滤模式）
- **交互式界面**:
  - 使用 ImGui 实现实时参数配置和渲染预览
  - 场景、相机、光照和材质的动态调整

## 快速开始

<details>
<summary><b>构建与运行</b></summary>

### 依赖项
- C++20 编译器（MSVC 2019+、GCC 10+、Clang 11+）
- CMake 3.12+
- OpenGL（系统自带）
- OpenMP（可选，用于多线程加速）

**注意**：GLFW 已包含在 `extern/glfw` 中，无需额外安装。

### Windows (MSVC)
```cmd
# 克隆仓库
git clone https://github.com/laoelaos/MicroRenderer.git
cd MicroRenderer
# 配置与编译
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
# 运行
.\bin\MicroRenderer.exe
```

### Linux / macOS
```bash
# 克隆仓库
git clone https://github.com/laoelaos/MicroRenderer.git
cd MicroRenderer
# 安装 OpenMP（可选，提升性能）
# Ubuntu/Debian:
sudo apt-get install libopenmp-dev
# macOS:
brew install libomp
# 配置与编译
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
# 运行
./bin/MicroRenderer
```

### 首次运行
程序启动时会弹出场景加载窗口，默认加载 `obj/default2.sc`。您也可以选择其他场景文件或自定义场景配置。

</details>

## 使用说明

1.  从 **加载场景** 面板加载一个 `.sc` 场景文件。
2.  在 **渲染设置** 下拉菜单中选择渲染模式
3.  在 **场景设置** 中实时调整相机、光照和材质参数，观察渲染结果。

## 渲染管线
`顶点数据` → `顶点着色` → `图元装配` → `裁剪` → `透视除法` → `视口变换` → `光栅化` → `片段着色` → `深度测试` → `帧缓冲`

## 未来计划
- [ ] PBR 材质系统
- [ ] 后处理效果 (SSAO, Bloom)
- [ ] 延迟渲染管线
- [ ] 软阴影 (PCF)

## 致谢
- [tinyrenderer](https://github.com/ssloy/tinyrenderer) - 项目灵感来源
- [Dear ImGui](https://github.com/ocornut/imgui) - UI 库
- [stb](https://github.com/nothings/stb) - 图像读写库
