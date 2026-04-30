#pragma once
#include <windows.h>

class Window {
public:
    ~Window() { Destroy(); }

    bool Create(HINSTANCE hInstance, int nCmdShow);
    void Destroy();

    HWND Handle() const { return m_hwnd; }
    int  Width()  const { return m_width; }
    int  Height() const { return m_height; }

    // Callbacks from main loop
    void (*OnTick)()   = nullptr;
    void (*OnClick)(int x, int y) = nullptr;

    // Tray icon callbacks
    void (*OnTrayIcon)(UINT msg) = nullptr;
    void (*OnTrayCommand)(int cmdId) = nullptr;

private:
    HWND m_hwnd  = nullptr;
    int  m_width  = 0;
    int  m_height = 0;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
