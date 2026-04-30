#include "TrayIcon.h"
#include "Types.h"
#include "resource.h"

bool TrayIcon::Create(HWND hwnd, HINSTANCE hInstance) {
    m_hwnd = hwnd;

    NOTIFYICONDATA nid = {};
    nid.cbSize           = sizeof(NOTIFYICONDATA);
    nid.hWnd             = hwnd;
    nid.uID              = 1;
    nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon            = (HICON)LoadImage(
        hInstance,
        MAKEINTRESOURCE(IDI_APP_ICON),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
    );
    if (!nid.hIcon) nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"Desktop Aquarium");

    Shell_NotifyIcon(NIM_DELETE, &nid);
    Shell_NotifyIcon(NIM_ADD, &nid);
    return true;
}

void TrayIcon::Destroy() {
    if (m_hwnd) {
        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd   = m_hwnd;
        nid.uID    = 1;
        Shell_NotifyIcon(NIM_DELETE, &nid);
        m_hwnd = nullptr;
    }
}

void TrayIcon::SetFeedMode(bool enabled) {
    m_feedMode = enabled;
}

void TrayIcon::ShowContextMenu(HWND hwnd) {
    HMENU menu = BuildMenu(hwnd);
    if (!menu) return;

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

HMENU TrayIcon::BuildMenu(HWND hwnd) {
    HMENU menu = ::CreatePopupMenu();
    if (!menu) return nullptr;

    AppendMenu(menu, MF_STRING | (m_feedMode ? MF_CHECKED : 0), ID_FEED_MODE, L"喂食模式");
    AppendMenu(menu, MF_SEPARATOR, 0, nullptr);

    HMENU countMenu = ::CreatePopupMenu();
    AppendMenu(countMenu, MF_STRING | (m_fishCount == 1  ? MF_CHECKED : 0), ID_FISH_COUNT_1,  L"1");
    AppendMenu(countMenu, MF_STRING | (m_fishCount == 5  ? MF_CHECKED : 0), ID_FISH_COUNT_5,  L"5");
    AppendMenu(countMenu, MF_STRING | (m_fishCount == 10 ? MF_CHECKED : 0), ID_FISH_COUNT_10, L"10");
    AppendMenu(countMenu, MF_STRING | (m_fishCount == 15 ? MF_CHECKED : 0), ID_FISH_COUNT_15, L"15");
    AppendMenu(countMenu, MF_STRING | (m_fishCount == 20 ? MF_CHECKED : 0), ID_FISH_COUNT_20, L"20");
    AppendMenu(menu, MF_POPUP, (UINT_PTR)countMenu, L"鱼的数量");

    HMENU schoolMenu = ::CreatePopupMenu();
    AppendMenu(schoolMenu, MF_STRING | (m_schoolCount == 1 ? MF_CHECKED : 0), ID_SCHOOL_COUNT_1, L"1 群");
    AppendMenu(schoolMenu, MF_STRING | (m_schoolCount == 2 ? MF_CHECKED : 0), ID_SCHOOL_COUNT_2, L"2 群");
    AppendMenu(schoolMenu, MF_STRING | (m_schoolCount == 3 ? MF_CHECKED : 0), ID_SCHOOL_COUNT_3, L"3 群");
    AppendMenu(schoolMenu, MF_STRING | (m_schoolCount == 4 ? MF_CHECKED : 0), ID_SCHOOL_COUNT_4, L"4 群");
    AppendMenu(menu, MF_POPUP, (UINT_PTR)schoolMenu, L"鱼群数量");

    HMENU speedMenu = ::CreatePopupMenu();
    AppendMenu(speedMenu, MF_STRING | (m_speedIndex == 0 ? MF_CHECKED : 0), ID_SPEED_SLOW, L"慢");
    AppendMenu(speedMenu, MF_STRING | (m_speedIndex == 1 ? MF_CHECKED : 0), ID_SPEED_MED,  L"中");
    AppendMenu(speedMenu, MF_STRING | (m_speedIndex == 2 ? MF_CHECKED : 0), ID_SPEED_FAST, L"快");
    AppendMenu(menu, MF_POPUP, (UINT_PTR)speedMenu, L"游动速度");

    HMENU sizeMenu = ::CreatePopupMenu();
    AppendMenu(sizeMenu, MF_STRING | (m_sizeIndex == 0 ? MF_CHECKED : 0), ID_SIZE_SMALL, L"小");
    AppendMenu(sizeMenu, MF_STRING | (m_sizeIndex == 1 ? MF_CHECKED : 0), ID_SIZE_MED,  L"中");
    AppendMenu(sizeMenu, MF_STRING | (m_sizeIndex == 2 ? MF_CHECKED : 0), ID_SIZE_LARGE, L"大");
    AppendMenu(menu, MF_POPUP, (UINT_PTR)sizeMenu, L"鱼的大小");

    AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(menu, MF_STRING, ID_QUIT, L"退出");

    return menu;
}

void TrayIcon::SetFishCount(int count) { m_fishCount = count; }
void TrayIcon::SetSchoolCount(int count) { m_schoolCount = count; }
void TrayIcon::SetSpeed(int index)     { m_speedIndex = index; }
void TrayIcon::SetSize(int index)      { m_sizeIndex = index; }
