#pragma once
#include <windows.h>
#include <d2d1.h>
#include <cmath>
#include <cstdlib>

// ============================================================
// 2D Vector
// ============================================================
struct Vec2 {
    float x, y;

    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2  operator+(Vec2 v) const { return {x + v.x, y + v.y}; }
    Vec2  operator-(Vec2 v) const { return {x - v.x, y - v.y}; }
    Vec2  operator*(float s) const { return {x * s, y * s}; }
    Vec2  operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(Vec2 v) { x += v.x; y += v.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float Length() const { return std::sqrt(x*x + y*y); }
    float LengthSq() const { return x*x + y*y; }
    Vec2  Normalized() const { float l = Length(); return l > 0 ? *this / l : Vec2(0,0); }
    float Dot(Vec2 v) const { return x*v.x + y*v.y; }
    void  Truncate(float max) { float l = Length(); if (l > max) { *this = *this * (max / l); } }
};

// ============================================================
// Color scheme for a fish
// ============================================================
struct FishColor {
    D2D1_COLOR_F body;        // main body
    D2D1_COLOR_F fin;         // tail / fins
    D2D1_COLOR_F accent;     // stripe / eye
};

inline D2D1_COLOR_F RGBf(float r, float g, float b, float a = 1.0f) {
    return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

enum class FishType {
    Round,     // 圆体鱼 — short oval, koi colors
    Torpedo,   // 流线鱼 — long torpedo, silver/blue
    Flat       // 扁体鱼 — tall compressed, yellow/black
};

constexpr int NUM_FISH_TYPES = 3;

inline FishColor GetFishColor(FishType type) {
    switch (type) {
        case FishType::Round: {
            // Small orange fish with light fins.
            int v = rand() % 3;
            if (v == 0)      return { RGBf(255, 134, 63),  RGBf(255, 181, 91),  RGBf(115, 70, 42) };
            else if (v == 1) return { RGBf(255, 164, 72),  RGBf(255, 210, 118), RGBf(125, 76, 40) };
            else             return { RGBf(238, 103, 54),  RGBf(255, 190, 96),  RGBf(100, 62, 45) };
        }
        case FishType::Torpedo: {
            // Tiny neon-blue schooling fish.
            int v = rand() % 3;
            if (v == 0)      return { RGBf(42, 183, 226), RGBf(243, 91, 105), RGBf(20, 96, 140) };
            else if (v == 1) return { RGBf(65, 201, 236), RGBf(255, 112, 92), RGBf(22, 110, 150) };
            else             return { RGBf(32, 165, 218), RGBf(250, 92, 130), RGBf(18, 86, 132) };
        }
        case FishType::Flat: {
            // Round sticker fish: mostly blue, sometimes yellow-green.
            int v = rand() % 4;
            if (v == 0)      return { RGBf(54, 176, 224), RGBf(132, 235, 246), RGBf(18, 92, 160) };
            else if (v == 1) return { RGBf(73, 194, 236), RGBf(145, 242, 250), RGBf(26, 108, 176) };
            else if (v == 2) return { RGBf(42, 157, 214), RGBf(124, 225, 242), RGBf(16, 82, 146) };
            else             return { RGBf(66, 197, 135), RGBf(255, 225, 98),  RGBf(40, 150, 102) };
        }
    }
    return {};
}

// ============================================================
// Constants
// ============================================================
constexpr int   TIMER_ID          = 1;
constexpr int   TIMER_INTERVAL_MS = 16;  // ~60fps
constexpr float MAX_FISH_SPEED    = 145.0f;  // px/s
constexpr float MAX_FISH_FORCE    = 58.0f;   // px/s²
constexpr float EDGE_BUFFER       = 120.0f;  // edge avoidance zone

// Boids radii
constexpr float SEPARATION_RADIUS = 42.0f;
constexpr float ALIGNMENT_RADIUS  = 185.0f;
constexpr float COHESION_RADIUS   = 230.0f;

// Boids weights
constexpr float SEPARATION_WEIGHT = 1.35f;
constexpr float ALIGNMENT_WEIGHT  = 1.45f;
constexpr float COHESION_WEIGHT   = 1.05f;

// Food
constexpr int   FOOD_PER_CLICK    = 9;
constexpr float FOOD_FALL_SPEED   = 24.0f;  // px/s
constexpr float FOOD_WOBBLE_AMP   = 9.0f;
constexpr float FOOD_WOBBLE_FREQ  = 1.35f;
constexpr float FOOD_RADIUS       = 3.0f;
constexpr float FOOD_DETECT_RADIUS = 340.0f;
constexpr float FOOD_EAT_RADIUS   = 18.0f;
constexpr float FOOD_COOLDOWN     = 0.3f;   // seconds between eats per fish
constexpr int   FOOD_MAX_LIFETIME_MS = 10000;

// Wave particles
constexpr int   WAVE_COUNT_MIN    = 30;
constexpr int   WAVE_COUNT_MAX    = 50;

// Custom window message for tray icon
constexpr UINT  WM_TRAYICON = WM_APP + 1;
