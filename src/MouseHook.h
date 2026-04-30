#pragma once
#include <windows.h>

class MouseHook {
public:
    ~MouseHook() { Destroy(); }

    bool Initialize();
    void Destroy();

    void SetFeedingMode(bool enabled) { m_feedingMode = enabled; }
    bool IsFeedingMode() const { return m_feedingMode; }

    // Called when a click is detected in feeding mode — receives screen coords
    void (*OnFeedClick)(int x, int y) = nullptr;

private:
    HHOOK  m_hook    = nullptr;
    bool   m_feedingMode = false;

    static LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);
};
