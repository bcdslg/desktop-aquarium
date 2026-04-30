#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d2d1.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>

#include "Window.h"
#include "Renderer.h"
#include "Fish.h"
#include "Food.h"
#include "MouseHook.h"
#include "TrayIcon.h"

// ─── Global state ──────────────────────────────────────────────
Window     g_window;
Renderer   g_renderer;
FoodSystem g_food;
MouseHook  g_mouseHook;
TrayIcon   g_trayIcon;

static std::vector<Fish> g_fish;
static int  g_targetFishCount = 15;
static int  g_targetSchoolCount = 3;
static float g_speedMultiplier = 1.0f;
static float g_sizeMultiplier  = 1.0f;

// ─── Wave particles (ambient) ──────────────────────────────────
struct WaveParticle {
    Vec2  pos;
    float speedX;
    float phase;
    float alpha;
    float radius;
};
static std::vector<WaveParticle> g_waveParticles;

static void InitWaveParticles(int width, int height) {
    int count = 30 + rand() % 21;
    g_waveParticles.resize(count);
    for (auto& w : g_waveParticles) {
        w.pos.x  = (float)(rand() % width);
        w.pos.y  = (float)(rand() % height);
        w.speedX = 5.0f + (float)(rand() % 11);
        w.phase  = (float)(rand() % 628) / 100.0f;
        w.alpha  = 0.05f + (float)(rand() % 10) / 100.0f;
        w.radius = 4.0f + (float)(rand() % 5);
    }
}

static void UpdateWaveParticles(float dt, int width, int height) {
    for (auto& w : g_waveParticles) {
        w.pos.x += w.speedX * dt;
        w.pos.y += sinf(w.phase) * 0.5f;
        w.phase += dt * 1.5f;

        if (w.pos.x > width + 20)  w.pos.x = -20;
        if (w.pos.x < -20)         w.pos.x = width + 20;
        if (w.pos.y < -20)         w.pos.y = height + 20;
        if (w.pos.y > height + 20) w.pos.y = -20;
    }
}

static void DrawWaveParticles(ID2D1DCRenderTarget* rt) {
    for (const auto& w : g_waveParticles) {
        ID2D1SolidColorBrush* brush = nullptr;
        rt->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, w.alpha), &brush);
        if (brush) {
            D2D1_ELLIPSE e = { D2D1::Point2F(w.pos.x, w.pos.y), w.radius, w.radius };
            rt->FillEllipse(e, brush);
            brush->Release();
        }
    }
}

// ─── Fish management ───────────────────────────────────────────
static void RespawnFish(int count, int width, int height) {
    g_fish.clear();
    g_fish.reserve(count);

    struct SchoolSeed {
        Vec2 center;
        Vec2 velocity;
        FishType type;
    };

    std::vector<SchoolSeed> schools;
    int soloCount = (std::max)(1, count / 3);
    int schoolFishCount = (std::max)(0, count - soloCount);
    int schoolCount = (std::max)(1, (std::min)(g_targetSchoolCount, (std::max)(1, schoolFishCount)));
    schools.reserve(schoolCount);
    for (int s = 0; s < schoolCount; ++s) {
        float angle = ((float)(rand() % 628) / 100.0f);
        float speed = 45.0f + (float)(rand() % 65);
        FishType type = (s % 3 == 0) ? FishType::Torpedo
                       : (s % 3 == 1) ? FishType::Round
                                      : FishType::Flat;
        schools.push_back({
            Vec2((float)(rand() % (width - 240) + 120),
                 (float)(rand() % (height - 240) + 120)),
            Vec2(cosf(angle) * speed, sinf(angle) * speed),
            type
        });
    }

    for (int i = 0; i < count; ++i) {
        bool solo = i >= schoolFishCount;
        if (solo) {
            float angle = ((float)(rand() % 628) / 100.0f);
            float speed = 35.0f + (float)(rand() % 90);
            Vec2 pos((float)(rand() % (width - 220) + 110),
                     (float)(rand() % (height - 220) + 110));

            if ((rand() % 100) < 45) {
                bool fromLeft = (rand() % 2) == 0;
                pos.x = fromLeft ? -80.0f : width + 80.0f;
                pos.y = (float)(rand() % (height - 180) + 90);
                angle = fromLeft ? ((float)(rand() % 80 - 40) / 100.0f)
                                 : 3.14159f + ((float)(rand() % 80 - 40) / 100.0f);
            }

            Vec2 vel(cosf(angle) * speed, sinf(angle) * speed);
            FishType type = (FishType)(rand() % NUM_FISH_TYPES);
            g_fish.emplace_back(type, pos, vel);
            g_fish.back().SetSize(g_sizeMultiplier);
            continue;
        }

        const auto& school = schools[i % schoolCount];
        FishType type = ((rand() % 100) < 82) ? school.type : (FishType)(rand() % NUM_FISH_TYPES);
        Vec2 pos(school.center.x + (float)(rand() % 180 - 90),
                 school.center.y + (float)(rand() % 110 - 55));
        Vec2 vel(school.velocity.x + (float)(rand() % 50 - 25),
                 school.velocity.y + (float)(rand() % 40 - 20));
        g_fish.emplace_back(type, pos, vel);
        g_fish.back().SetSize(g_sizeMultiplier);
    }
}

// ─── Timer tick ────────────────────────────────────────────────
static void OnFrameTick() {
    float dt = TIMER_INTERVAL_MS / 1000.0f;
    int w = g_renderer.Width();
    int h = g_renderer.Height();

    // ── Simulate ──
    // Gather food positions
    static thread_local std::vector<Vec2> foodPos;
    foodPos.clear();
    for (int i = 0; i < g_food.Count(); ++i)
        foodPos.push_back(g_food.Particles()[i].position);

    static thread_local std::vector<Vec2> fishPos;
    static thread_local std::vector<Vec2> fishVel;
    fishPos.clear();
    fishVel.clear();
    fishPos.reserve(g_fish.size());
    fishVel.reserve(g_fish.size());
    for (const auto& f : g_fish) {
        fishPos.push_back(f.Position());
        fishVel.push_back(f.Velocity());
    }

    // Update fish
    for (int i = 0; i < (int)g_fish.size(); ++i) {
        // Edge avoidance
        Vec2 avoid;
        constexpr float B = EDGE_BUFFER;
        Vec2 p = g_fish[i].Position();
        if (p.x < B)      avoid.x += (B - p.x) / B;
        if (p.x > w - B)  avoid.x -= (p.x - (w - B)) / B;
        if (p.y < B)      avoid.y += (B - p.y) / B;
        if (p.y > h - B)  avoid.y -= (p.y - (h - B)) / B;
        avoid = avoid * MAX_FISH_SPEED * 2.0f;

        BoidsContext ctx;
        ctx.positions      = fishPos.data();
        ctx.velocities     = fishVel.data();
        ctx.count          = (int)g_fish.size();
        ctx.selfIndex      = i;
        ctx.foodPositions  = foodPos.data();
        ctx.foodCount      = (int)foodPos.size();
        ctx.foodDetectRadius = FOOD_DETECT_RADIUS;
        ctx.avoidForce     = avoid;

        g_fish[i].Update(dt * g_speedMultiplier, ctx);
    }

    // Update food + collision
    g_food.Update(dt);
    for (int fi = 0; fi < (int)g_fish.size(); ++fi) {
        Vec2 fp = g_fish[fi].Position();
        for (int pi = g_food.Count() - 1; pi >= 0; --pi) {
            if ((fp - g_food.Particles()[pi].position).Length() < FOOD_EAT_RADIUS) {
                g_food.Eat(pi);
                break;
            }
        }
    }

    UpdateWaveParticles(dt, w, h);

    // ── Render ──
    g_renderer.BeginFrame();
    auto* rt = g_renderer.RT();

    rt->Clear(D2D1::ColorF(0.02f, 0.03f, 0.06f, 0.05f));

    DrawWaveParticles(rt);
    g_food.Draw(rt);

    for (auto& fish : g_fish)
        fish.Draw(rt);

    g_renderer.EndFrame();
}

// ─── Tray icon handlers ────────────────────────────────────────
static void ApplySize(int index) {
    g_trayIcon.SetSize(index);
    switch (index) {
        case 0: g_sizeMultiplier = 0.6f; break;
        case 1: g_sizeMultiplier = 1.0f; break;
        case 2: g_sizeMultiplier = 1.5f; break;
    }
    for (auto& f : g_fish)
        f.SetSize(g_sizeMultiplier);
}

static void SetFishCount(int count) {
    g_targetFishCount = count;
    g_trayIcon.SetFishCount(count);
    RespawnFish(count, g_window.Width(), g_window.Height());
}

static void SetSchoolCount(int count) {
    g_targetSchoolCount = count;
    g_trayIcon.SetSchoolCount(count);
    RespawnFish(g_targetFishCount, g_window.Width(), g_window.Height());
}

static void OnTrayCommand(int cmdId) {
    switch (cmdId) {
        case ID_FEED_MODE: {
            bool on = !g_mouseHook.IsFeedingMode();
            g_mouseHook.SetFeedingMode(on);
            g_trayIcon.SetFeedMode(on);
            break;
        }
        case ID_FISH_COUNT_1:  SetFishCount(1);  break;
        case ID_FISH_COUNT_5:  SetFishCount(5);  break;
        case ID_FISH_COUNT_10: SetFishCount(10); break;
        case ID_FISH_COUNT_15: SetFishCount(15); break;
        case ID_FISH_COUNT_20: SetFishCount(20); break;
        case ID_SCHOOL_COUNT_1: SetSchoolCount(1); break;
        case ID_SCHOOL_COUNT_2: SetSchoolCount(2); break;
        case ID_SCHOOL_COUNT_3: SetSchoolCount(3); break;
        case ID_SCHOOL_COUNT_4: SetSchoolCount(4); break;
        case ID_SPEED_SLOW:  g_trayIcon.SetSpeed(0); g_speedMultiplier = 0.5f; break;
        case ID_SPEED_MED:   g_trayIcon.SetSpeed(1); g_speedMultiplier = 1.0f; break;
        case ID_SPEED_FAST:  g_trayIcon.SetSpeed(2); g_speedMultiplier = 2.0f; break;
        case ID_SIZE_SMALL:  ApplySize(0); break;
        case ID_SIZE_MED:    ApplySize(1); break;
        case ID_SIZE_LARGE:  ApplySize(2); break;
        case ID_QUIT:        PostQuitMessage(0); break;
    }
}

static void OnTrayIcon(UINT msg) {
    if (msg == WM_LBUTTONDBLCLK) {
        bool on = !g_mouseHook.IsFeedingMode();
        g_mouseHook.SetFeedingMode(on);
        g_trayIcon.SetFeedMode(on);
    } else if (msg == WM_RBUTTONUP) {
        g_trayIcon.ShowContextMenu(g_window.Handle());
    }
}

static void OnFeedClick(int x, int y) {
    g_food.Spawn((float)x, (float)y, FOOD_PER_CLICK);
}

// ─── WinMain ───────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    srand((unsigned)time(nullptr));
    SetProcessDPIAware();

    g_window.OnTick       = OnFrameTick;
    g_window.OnTrayIcon   = OnTrayIcon;
    g_window.OnTrayCommand = OnTrayCommand;

    if (!g_window.Create(hInstance, nCmdShow))     return 11;
    if (!g_renderer.Initialize(g_window.Handle(),
                               g_window.Width(),
                               g_window.Height())) return 12;
    if (!g_trayIcon.Create(g_window.Handle(), hInstance)) return 13;
    if (!g_mouseHook.Initialize())                 return 14;

    g_mouseHook.OnFeedClick = OnFeedClick;
    RespawnFish(g_targetFishCount, g_window.Width(), g_window.Height());
    InitWaveParticles(g_window.Width(), g_window.Height());

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup handled by destructors
    return 0;
}
