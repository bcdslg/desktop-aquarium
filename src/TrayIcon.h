#pragma once
#include <windows.h>

// Menu command IDs
enum TrayCommand {
    ID_FEED_MODE   = 1001,
    ID_FISH_COUNT_1   = 1010,
    ID_FISH_COUNT_5   = 1011,
    ID_FISH_COUNT_10  = 1012,
    ID_FISH_COUNT_15  = 1013,
    ID_FISH_COUNT_20  = 1014,
    ID_SCHOOL_COUNT_1 = 1015,
    ID_SCHOOL_COUNT_2 = 1016,
    ID_SCHOOL_COUNT_3 = 1017,
    ID_SCHOOL_COUNT_4 = 1018,
    ID_SPEED_SLOW  = 1020,
    ID_SPEED_MED   = 1021,
    ID_SPEED_FAST  = 1022,
    ID_SIZE_SMALL  = 1030,
    ID_SIZE_MED    = 1031,
    ID_SIZE_LARGE  = 1032,
    ID_QUIT        = 1999,
};

class TrayIcon {
public:
    ~TrayIcon() { Destroy(); }

    bool Create(HWND hwnd, HINSTANCE hInstance);
    void Destroy();

    void SetFeedMode(bool enabled);
    void ShowContextMenu(HWND hwnd);

    // Setters for menu state
    void SetFishCount(int count);
    void SetSchoolCount(int count);
    void SetSpeed(int index);   // 0=slow, 1=med, 2=fast
    void SetSize(int index);    // 0=small, 1=med, 2=large

private:
    HWND m_hwnd = nullptr;
    bool m_feedMode = false;
    int  m_fishCount = 15;
    int  m_schoolCount = 3;
    int  m_speedIndex = 1;  // medium default
    int  m_sizeIndex = 1;   // medium default

    HMENU BuildMenu(HWND hwnd);
};
