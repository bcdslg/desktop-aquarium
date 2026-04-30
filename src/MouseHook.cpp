#include "MouseHook.h"

static MouseHook* g_hookInstance = nullptr;

bool MouseHook::Initialize() {
    g_hookInstance = this;
    m_hook = SetWindowsHookEx(WH_MOUSE_LL, HookProc, GetModuleHandle(nullptr), 0);
    return m_hook != nullptr;
}

void MouseHook::Destroy() {
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }
    g_hookInstance = nullptr;
}

LRESULT CALLBACK MouseHook::HookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0 && g_hookInstance && g_hookInstance->m_feedingMode) {
        if (wParam == WM_LBUTTONDOWN) {
            auto* info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
            if (g_hookInstance->OnFeedClick) {
                g_hookInstance->OnFeedClick(info->pt.x, info->pt.y);
            }
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}
