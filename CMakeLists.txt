cmake_minimum_required(VERSION 3.20)
project(DesktopAquarium VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(DesktopAquarium WIN32
    src/main.cpp
    src/Window.cpp
    src/Renderer.cpp
    src/Fish.cpp
    src/Food.cpp
    src/MouseHook.cpp
    src/TrayIcon.cpp
    resources/resources.rc
)

target_compile_definitions(DesktopAquarium PRIVATE
    UNICODE _UNICODE
    NOMINMAX
)

target_link_libraries(DesktopAquarium PRIVATE
    gdi32
    user32
    d2d1
)

target_include_directories(DesktopAquarium PRIVATE src)
