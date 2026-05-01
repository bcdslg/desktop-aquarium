# Desktop Aquarium

中文 | [English](README.md)

Desktop Aquarium 是一个轻量的 Windows 桌面伴侣，在桌面上渲染游动的鱼。基于透明分层窗口、Direct2D 绘制，支持鱼群群聚、独立游鱼、食物粒子和托盘菜单快捷控制。

![Desktop Aquarium icon](resources/app.ico)

## 功能

- 透明置顶桌面水族箱。
- 贴纸风格的鱼，不同大小、颜色和游动个性。
- 混合行为：鱼群群聚 + 独立游动的鱼。
- 喂食模式：点击投放食物，附近感兴趣的鱼会加速游向食物。
- 托盘菜单控制：鱼数量、鱼群数量、速度、大小、喂食模式、退出。
- 原生 Win32/C++ + Direct2D 实现。

## 下载

无需安装 — 从 **[Releases](https://github.com/bcdslg/desktop-aquarium/releases)** 页面下载最新的 `DesktopAquarium.exe`，直接运行即可。

## 环境要求

仅从源码构建时需要：

- Windows 10 或 Windows 11。
- Visual Studio 2022/2026（含 C++ 桌面开发工具），或兼容的 MSVC 工具链。
- CMake 3.20 或更新版本。

## 构建

在 Visual Studio 开发者命令提示符中执行：

```bat
cmake -S . -B build
cmake --build build --config Release
```

或直接运行：

```bat
build.bat
```

生成的可执行文件位于：

```text
build\Release\DesktopAquarium.exe
```

## 使用

运行 `DesktopAquarium.exe`，通过托盘图标菜单：

- 开关喂食模式。
- 调整鱼数量。
- 调整鱼群数量。
- 调整游动速度。
- 调整鱼大小。
- 退出程序。

开启喂食模式后，点击桌面即可投放食物。

## 项目结构

```text
src/          Win32、Direct2D、鱼、食物、托盘、鼠标钩子代码
resources/    应用图标、清单和 Windows 资源文件
CMakeLists.txt
build.bat
```

## 说明

仅支持 Windows。程序创建透明穿透点击的覆盖窗口，并使用全局鼠标钩子实现喂食模式。

## 许可证

MIT License。详见 [LICENSE](LICENSE)。
