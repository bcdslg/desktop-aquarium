#include "Window.h"
#include "Types.h"
#include "resource.h"

bool Window::Create(HINSTANCE hInstance, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"DesktopAquarium";

    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));

    if (!RegisterClass(&wc)) {
        return false;
    }

    m_width  = GetSystemMetrics(SM_CXSCREEN);
    m_height = GetSystemMetrics(SM_CYSCREEN);

    m_hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"DesktopAquarium",
        WS_POPUP,
        0, 0, m_width, m_height,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hwnd) return false;

    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(m_hwnd, nCmdShow);

    // Start 60fps timer
    SetTimer(m_hwnd, TIMER_ID, TIMER_INTERVAL_MS, nullptr);

    return true;
}

void Window::Destroy() {
    if (m_hwnd) {
        KillTimer(m_hwnd, TIMER_ID);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* self = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_TIMER:
            if (self && self->OnTick) self->OnTick();
            return 0;

        case WM_TRAYICON:
            if (self && self->OnTrayIcon) self->OnTrayIcon((UINT)lParam);
            return 0;

        case WM_COMMAND:
            if (self && self->OnTrayCommand) self->OnTrayCommand((int)LOWORD(wParam));
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
