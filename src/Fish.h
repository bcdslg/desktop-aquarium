#pragma once
#include "Types.h"
#include <d2d1.h>

struct BoidsContext {
    const Vec2* positions;     // array of all fish positions
    const Vec2* velocities;    // array of all fish velocities
    const int*  schoolIds;     // array of school IDs per fish (-1 = solo)
    int   count;               // total fish count
    int   selfIndex;           // index of this fish in the arrays
    int   selfSchoolId;        // school ID of this fish
    const Vec2* foodPositions; // array of food particle positions
    int   foodCount;           // number of active food particles
    float foodDetectRadius;
    Vec2  avoidForce;          // edge-avoidance force pre-computed for this fish
    bool  freeSwim = false;    // when true, fish don't school (no alignment/cohesion)
};

class Fish {
public:
    Fish(FishType type, Vec2 pos, Vec2 vel);
    ~Fish();
    Fish(Fish&& other) noexcept;
    Fish& operator=(Fish&& other) noexcept;
    Fish(const Fish&) = delete;
    Fish& operator=(const Fish&) = delete;

    void Update(float dt, const BoidsContext& ctx);
    void Draw(ID2D1DCRenderTarget* rt) const;

    const Vec2& Position() const { return m_position; }
    const Vec2& Velocity() const { return m_velocity; }
    int SchoolId() const { return m_schoolId; }
    void SetSize(float s) { m_uiScale = s; }
    void SetSchoolId(int id) { m_schoolId = id; }

private:
    Vec2 m_position;
    Vec2 m_velocity;
    float m_tailPhase = 0.0f;   // [0, 2π) for tail oscillation
    float m_wanderPhase = 0.0f;
    float m_wanderRate = 1.0f;
    float m_personality = 1.0f;
    float m_foodInterest = 1.0f;
    float m_sprintBoost = 0.0f;
    float m_baseScale = 1.0f;
    float m_uiScale = 1.0f;
    bool  m_hasStripe = false;
    int   m_schoolId = -1;      // -1 = solo, >= 0 = school index

    FishType     m_type;
    FishColor    m_color;
    ID2D1LinearGradientBrush* m_bodyGradient = nullptr;

    D2D1_POINT_2F m_baseBody[48];   // base body shape for per-frame undulation

    void ApplySteering(Vec2 force, float dt);

    Vec2 Separation(const BoidsContext& ctx) const;
    Vec2 Alignment(const BoidsContext& ctx) const;
    Vec2 Cohesion(const BoidsContext& ctx) const;
    Vec2 SeekFood(const BoidsContext& ctx) const;

    static ID2D1Factory* GetFactory(ID2D1DCRenderTarget* rt);
};
