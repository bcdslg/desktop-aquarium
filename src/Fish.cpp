#include "Fish.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>

// ─── Smooth body generation ────────────────────────────────────

struct FishShape {
    int              bodyCount;
    const D2D1_POINT_2F* body;
    float            eyeX, eyeY, eyeR;
};

static float BodyProfile(float t, float fullness) {
    if (t < 0.35f) {
        float s = t / 0.35f;
        return fullness * s * (2.0f - s);
    } else {
        float s = (t - 0.35f) / 0.65f;
        return fullness * (1.0f - s * s) * (1.0f - 0.35f * s);
    }
}

static void GenBody(D2D1_POINT_2F* pts, int count, float halfLen, float peakHeight, float nosePointy) {
    int half = count / 2;
    for (int i = 0; i < half; ++i) {
        float t = (float)i / (half - 1);
        float hw = BodyProfile(1.0f - t, peakHeight);
        if (t > 0.75f) {
            float ns = (t - 0.75f) / 0.25f;
            hw *= (1.0f - ns) + (1.0f - ns * ns) * nosePointy;
        }
        float x = -halfLen + 2.0f * halfLen * t;
        pts[i] = D2D1::Point2F(x, -hw);
        pts[count - 1 - i] = D2D1::Point2F(x, hw);
    }
}

// ─── Static body templates ──────────────────────────────────────
#define BODY_PTS 48
static D2D1_POINT_2F g_roundBody[BODY_PTS];
static D2D1_POINT_2F g_torpedoBody[BODY_PTS];
static D2D1_POINT_2F g_flatBody[BODY_PTS];
static bool g_bodiesGen = false;

static void EnsureBodies() {
    if (g_bodiesGen) return;
    g_bodiesGen = true;

    GenBody(g_roundBody, BODY_PTS, 65.0f, 28.0f, 0.6f);
    GenBody(g_torpedoBody, BODY_PTS, 80.0f, 18.0f, 0.9f);
    GenBody(g_flatBody, BODY_PTS, 60.0f, 35.0f, 0.4f);
}

static const FishShape kRoundShape = {
    BODY_PTS, g_roundBody,
    50, -4, 3.5f,
};
static const FishShape kTorpedoShape = {
    BODY_PTS, g_torpedoBody,
    65, -3, 3.0f,
};
static const FishShape kFlatShape = {
    BODY_PTS, g_flatBody,
    45, -5, 3.5f,
};

static const FishShape* GetShape(FishType type) {
    EnsureBodies();
    switch (type) {
        case FishType::Round:   return &kRoundShape;
        case FishType::Torpedo: return &kTorpedoShape;
        case FishType::Flat:    return &kFlatShape;
    }
    return &kRoundShape;
}

// ─── Tail fin geometry ──────────────────────────────────────────
static void BuildTailGeometry(ID2D1GeometrySink* sink, const FishShape* shape,
                               float tailPhase, float halfLen) {
    float tailAngle = sinf(tailPhase) * 0.3f;
    float c = cosf(tailAngle), s = sinf(tailAngle);

    float baseHW = BodyProfile(1.0f, 28.0f) * 0.2f;
    baseHW = (shape->eyeR > 3.2f) ? baseHW * 1.2f : baseHW * 0.8f;

    float mx = -halfLen;
    float tailLen = 35.0f;
    float forkSpread = 12.0f;
    float notchInset = 15.0f;

    float ptsX[] = {
        mx,
        mx - tailLen,
        mx - tailLen + notchInset,
        mx - tailLen,
        mx,
    };
    float ptsY[] = {
        -baseHW,
        -baseHW - forkSpread,
        0,
        baseHW + forkSpread,
        baseHW,
    };

    sink->BeginFigure(D2D1::Point2F(
        ptsX[0] * c - ptsY[0] * s + mx * (1 - c),
        (ptsX[0] - mx) * s + ptsY[0] * c
    ), D2D1_FIGURE_BEGIN_FILLED);
    for (int i = 1; i < 5; ++i) {
        sink->AddLine(D2D1::Point2F(
            ptsX[i] * c - ptsY[i] * s + mx * (1 - c),
            (ptsX[i] - mx) * s + ptsY[i] * c
        ));
    }
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
}

// ─── Color helpers ──────────────────────────────────────────────
static FishColor VaryColor(FishColor base) {
    float shift = 0.85f + (float)(rand() % 30) / 100.0f;
    auto vary = [&](D2D1_COLOR_F c) -> D2D1_COLOR_F {
        return D2D1::ColorF(c.r * shift, c.g * shift, c.b * shift, c.a);
    };
    return { vary(base.body), vary(base.fin), base.accent };
}

static D2D1_COLOR_F Brighten(D2D1_COLOR_F c, float f) {
    return D2D1::ColorF((std::min)(1.0f, c.r * f), (std::min)(1.0f, c.g * f), (std::min)(1.0f, c.b * f), c.a);
}
static D2D1_COLOR_F Darken(D2D1_COLOR_F c, float f) {
    return D2D1::ColorF(c.r * f, c.g * f, c.b * f, c.a);
}

static float GetHalfLen(FishType type) {
    switch (type) {
        case FishType::Round:   return 65.0f;
        case FishType::Torpedo: return 80.0f;
        case FishType::Flat:    return 60.0f;
    }
    return 65.0f;
}

static D2D1_COLOR_F WithAlpha(D2D1_COLOR_F c, float a) {
    c.a = a;
    return c;
}

static void FillPoly(ID2D1DCRenderTarget* rt, ID2D1Factory* factory,
                     const D2D1_POINT_2F* pts, int count, D2D1_COLOR_F color) {
    ID2D1PathGeometry* geo = nullptr;
    if (FAILED(factory->CreatePathGeometry(&geo)) || !geo) return;

    ID2D1GeometrySink* sink = nullptr;
    if (SUCCEEDED(geo->Open(&sink)) && sink) {
        sink->BeginFigure(pts[0], D2D1_FIGURE_BEGIN_FILLED);
        for (int i = 1; i < count; ++i) sink->AddLine(pts[i]);
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        sink->Close();
        sink->Release();

        ID2D1SolidColorBrush* brush = nullptr;
        if (SUCCEEDED(rt->CreateSolidColorBrush(color, &brush)) && brush) {
            rt->FillGeometry(geo, brush);
            brush->Release();
        }
    }
    geo->Release();
}

static void FillTriangle(ID2D1DCRenderTarget* rt, ID2D1Factory* factory,
                         D2D1_POINT_2F a, D2D1_POINT_2F b, D2D1_POINT_2F c,
                         D2D1_COLOR_F color) {
    D2D1_POINT_2F pts[] = { a, b, c };
    FillPoly(rt, factory, pts, 3, color);
}

static void FillStickerEllipse(ID2D1DCRenderTarget* rt, D2D1_POINT_2F center,
                               float rx, float ry, D2D1_COLOR_F color) {
    ID2D1SolidColorBrush* brush = nullptr;
    if (SUCCEEDED(rt->CreateSolidColorBrush(color, &brush)) && brush) {
        rt->FillEllipse(D2D1::Ellipse(center, rx, ry), brush);
        brush->Release();
    }
}

// ─── Fish implementation ───────────────────────────────────────
Fish::Fish(FishType type, Vec2 pos, Vec2 vel)
    : m_position(pos), m_velocity(vel), m_type(type) {
    m_tailPhase = (float)(rand() % 628) / 100.0f;
    m_wanderPhase = (float)(rand() % 628) / 100.0f;
    m_wanderRate = 0.65f + (float)(rand() % 90) / 100.0f;
    m_personality = 0.75f + (float)(rand() % 70) / 100.0f;
    m_foodInterest = (float)(rand() % 100) / 100.0f;
    m_hasStripe = (rand() % 100) < 72;

    if (type == FishType::Torpedo) {
        m_baseScale = 0.18f + (float)(rand() % 22) / 100.0f;
    } else if (type == FishType::Round) {
        m_baseScale = 0.24f + (float)(rand() % 28) / 100.0f;
    } else {
        m_baseScale = 0.28f + (float)(rand() % 34) / 100.0f;
    }
    m_color = VaryColor(GetFishColor(type));

    // Copy base body template for per-frame undulation
    const FishShape* shape = GetShape(m_type);
    memcpy(m_baseBody, shape->body, sizeof(D2D1_POINT_2F) * BODY_PTS);
}

Fish::~Fish() {
    if (m_bodyGradient) { m_bodyGradient->Release(); m_bodyGradient = nullptr; }
}

Fish::Fish(Fish&& other) noexcept
    : m_position(other.m_position),
      m_velocity(other.m_velocity),
      m_tailPhase(other.m_tailPhase),
      m_wanderPhase(other.m_wanderPhase),
      m_wanderRate(other.m_wanderRate),
      m_personality(other.m_personality),
      m_foodInterest(other.m_foodInterest),
      m_sprintBoost(other.m_sprintBoost),
      m_baseScale(other.m_baseScale),
      m_uiScale(other.m_uiScale),
      m_hasStripe(other.m_hasStripe),
      m_type(other.m_type),
      m_color(other.m_color),
      m_bodyGradient(other.m_bodyGradient) {
    memcpy(m_baseBody, other.m_baseBody, sizeof(m_baseBody));
    other.m_bodyGradient = nullptr;
}

Fish& Fish::operator=(Fish&& other) noexcept {
    if (this != &other) {
        if (m_bodyGradient) m_bodyGradient->Release();
        m_position  = other.m_position;
        m_velocity  = other.m_velocity;
        m_tailPhase = other.m_tailPhase;
        m_wanderPhase = other.m_wanderPhase;
        m_wanderRate = other.m_wanderRate;
        m_personality = other.m_personality;
        m_foodInterest = other.m_foodInterest;
        m_sprintBoost = other.m_sprintBoost;
        m_baseScale = other.m_baseScale;
        m_uiScale = other.m_uiScale;
        m_hasStripe = other.m_hasStripe;
        m_type      = other.m_type;
        m_color     = other.m_color;
        memcpy(m_baseBody, other.m_baseBody, sizeof(m_baseBody));
        m_bodyGradient = other.m_bodyGradient;
        other.m_bodyGradient = nullptr;
    }
    return *this;
}

void Fish::Update(float dt, const BoidsContext& ctx) {
    Vec2 sep   = Separation(ctx) * SEPARATION_WEIGHT;
    Vec2 ali   = Alignment(ctx)  * ALIGNMENT_WEIGHT;
    Vec2 coh   = Cohesion(ctx)   * COHESION_WEIGHT;
    Vec2 food  = SeekFood(ctx);
    Vec2 avoid = ctx.avoidForce;
    Vec2 dir = m_velocity.Normalized();
    Vec2 side(-dir.y, dir.x);
    float drift = sinf(m_wanderPhase) * 18.0f * m_personality;
    Vec2 wander = side * drift + dir * (cosf(m_wanderPhase * 0.57f) * 8.0f);

    if (food.LengthSq() > 0.0f) {
        m_sprintBoost = (std::max)(m_sprintBoost, 0.38f + m_foodInterest * 0.55f);
    } else {
        m_sprintBoost = (std::max)(0.0f, m_sprintBoost - dt * 1.6f);
    }

    ApplySteering(sep + ali + coh + food * 2.7f + avoid + wander, dt);

    m_position += m_velocity * dt;

    float speed = m_velocity.Length();
    float flapRate = 3.0f + (speed / MAX_FISH_SPEED) * 7.0f;
    m_tailPhase += dt * flapRate * 6.2832f;
    m_wanderPhase += dt * m_wanderRate * 2.4f;
}

void Fish::Draw(ID2D1DCRenderTarget* rt) const {
    auto* factory = GetFactory(rt);
    if (!factory) return;
    float stickerAngle = std::atan2(m_velocity.y, m_velocity.x);
    float stickerScale = m_baseScale * m_uiScale;
    rt->SetTransform(
        D2D1::Matrix3x2F::Scale(stickerScale, stickerScale) *
        D2D1::Matrix3x2F::Rotation(stickerAngle * 180.0f / 3.14159f) *
        D2D1::Matrix3x2F::Translation(m_position.x, m_position.y)
    );

    float wag = sinf(m_tailPhase) * 7.0f;
    D2D1_COLOR_F body = WithAlpha(m_color.body, 0.88f);
    D2D1_COLOR_F fin = WithAlpha(m_color.fin, 0.72f);
    D2D1_COLOR_F accent = WithAlpha(m_color.accent, 0.72f);

    if (m_type == FishType::Torpedo) {
        FillTriangle(rt, factory, D2D1::Point2F(-43, -7), D2D1::Point2F(-70, -17 + wag), D2D1::Point2F(-58, wag * 0.4f), fin);
        FillTriangle(rt, factory, D2D1::Point2F(-43, 7), D2D1::Point2F(-70, 17 + wag), D2D1::Point2F(-58, wag * 0.4f), fin);
        FillStickerEllipse(rt, D2D1::Point2F(0, 0), 48, 11, body);
        FillTriangle(rt, factory, D2D1::Point2F(-6, -9), D2D1::Point2F(10, -22), D2D1::Point2F(18, -7), WithAlpha(Brighten(body, 1.25f), 0.38f));
        FillTriangle(rt, factory, D2D1::Point2F(-4, 9), D2D1::Point2F(12, 22), D2D1::Point2F(20, 7), WithAlpha(Brighten(body, 1.25f), 0.32f));
        FillStickerEllipse(rt, D2D1::Point2F(35, -3), 3.0f, 3.0f, D2D1::ColorF(1, 1, 1, 0.82f));
        FillStickerEllipse(rt, D2D1::Point2F(36, -3), 1.3f, 1.3f, accent);
    } else if (m_type == FishType::Round) {
        FillTriangle(rt, factory, D2D1::Point2F(-50, -12), D2D1::Point2F(-82, -27 + wag), D2D1::Point2F(-66, wag * 0.35f), fin);
        FillTriangle(rt, factory, D2D1::Point2F(-50, 12), D2D1::Point2F(-82, 27 + wag), D2D1::Point2F(-66, wag * 0.35f), fin);
        FillStickerEllipse(rt, D2D1::Point2F(0, 0), 52, 20, body);
        if (m_hasStripe) {
            FillStickerEllipse(rt, D2D1::Point2F(-24, 0), 4.4f, 19.0f, WithAlpha(Brighten(fin, 1.25f), 0.75f));
            FillStickerEllipse(rt, D2D1::Point2F(0, 0), 4.0f, 18.0f, WithAlpha(Brighten(fin, 1.15f), 0.65f));
            FillStickerEllipse(rt, D2D1::Point2F(24, 0), 3.6f, 15.0f, WithAlpha(Brighten(fin, 1.1f), 0.55f));
        }
        FillStickerEllipse(rt, D2D1::Point2F(37, -5), 3.2f, 3.2f, D2D1::ColorF(1, 1, 1, 0.82f));
        FillStickerEllipse(rt, D2D1::Point2F(38, -5), 1.4f, 1.4f, accent);
    } else {
        FillTriangle(rt, factory, D2D1::Point2F(-38, -14), D2D1::Point2F(-68, -32 + wag), D2D1::Point2F(-58, wag * 0.35f), fin);
        FillTriangle(rt, factory, D2D1::Point2F(-38, 14), D2D1::Point2F(-68, 32 + wag), D2D1::Point2F(-58, wag * 0.35f), fin);
        FillTriangle(rt, factory, D2D1::Point2F(-10, -26), D2D1::Point2F(5, -48), D2D1::Point2F(14, -24), WithAlpha(Brighten(fin, 1.12f), 0.42f));
        FillTriangle(rt, factory, D2D1::Point2F(-8, 26), D2D1::Point2F(8, 48), D2D1::Point2F(16, 24), WithAlpha(Brighten(fin, 1.1f), 0.34f));
        FillStickerEllipse(rt, D2D1::Point2F(0, 0), 42, 31, body);
        if (m_hasStripe) {
            D2D1_POINT_2F s1[] = { D2D1::Point2F(-20, -28), D2D1::Point2F(-11, -29), D2D1::Point2F(4, 27), D2D1::Point2F(-6, 29) };
            D2D1_POINT_2F s2[] = { D2D1::Point2F(10, -26), D2D1::Point2F(18, -24), D2D1::Point2F(27, 18), D2D1::Point2F(18, 23) };
            FillPoly(rt, factory, s1, 4, accent);
            FillPoly(rt, factory, s2, 4, accent);
        }
        FillStickerEllipse(rt, D2D1::Point2F(28, -6), 3.4f, 3.4f, D2D1::ColorF(1, 1, 1, 0.84f));
        FillStickerEllipse(rt, D2D1::Point2F(29, -6), 1.5f, 1.5f, D2D1::ColorF(0.05f, 0.09f, 0.14f, 0.86f));
    }

    rt->SetTransform(D2D1::Matrix3x2F::Identity());
    factory->Release();
    return;

    // ── Lazy gradient init (cylindrical shading) ──
    if (!m_bodyGradient) {
        auto* self = const_cast<Fish*>(this);
        ID2D1GradientStopCollection* stops = nullptr;
        D2D1_GRADIENT_STOP stopArray[] = {
            {0.0f,  Darken(m_color.body, 0.4f)},   // top edge shadow
            {0.10f, Brighten(m_color.body, 2.2f)},  // specular highlight
            {0.25f, Brighten(m_color.body, 1.4f)},  // upper body
            {0.50f, m_color.body},                   // midline
            {0.75f, Darken(m_color.body, 0.5f)},     // lower body
            {1.0f,  Darken(m_color.body, 0.2f)},     // bottom edge
        };
        if (SUCCEEDED(rt->CreateGradientStopCollection(stopArray, 6, &stops))) {
            D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props = {
                D2D1::Point2F(0, -35),
                D2D1::Point2F(0, 35)
            };
            rt->CreateLinearGradientBrush(props, stops, &self->m_bodyGradient);
            stops->Release();
        }
    }

    const FishShape* shape = GetShape(m_type);
    float halfLen = GetHalfLen(m_type);

    float angle = std::atan2(m_velocity.y, m_velocity.x);
    float renderScale = m_baseScale * m_uiScale;
    auto transform =
        D2D1::Matrix3x2F::Scale(renderScale, renderScale) *
        D2D1::Matrix3x2F::Rotation(angle * 180.0f / 3.14159f) *
        D2D1::Matrix3x2F::Translation(m_position.x, m_position.y);
    rt->SetTransform(transform);

    // ── Body geometry (deformed with undulation wave) ──
    ID2D1PathGeometry* bodyGeo = nullptr;
    HRESULT hr = factory->CreatePathGeometry(&bodyGeo);
    if (SUCCEEDED(hr) && bodyGeo) {
        ID2D1GeometrySink* sink = nullptr;
        hr = bodyGeo->Open(&sink);
        if (SUCCEEDED(hr) && sink) {
            D2D1_POINT_2F pts[BODY_PTS];
            for (int i = 0; i < BODY_PTS; ++i) {
                float x = m_baseBody[i].x;
                float y = m_baseBody[i].y;
                float t = (x + halfLen) / (2.0f * halfLen); // 0=tail base, 1=nose
                float amp = 7.0f * (1.0f - t);               // max at tail, zero at nose
                y += amp * sinf(t * 3.14159f * 1.5f - m_tailPhase);
                pts[i] = D2D1::Point2F(x, y);
            }
            sink->BeginFigure(pts[0], D2D1_FIGURE_BEGIN_FILLED);
            for (int i = 1; i < BODY_PTS; ++i)
                sink->AddLine(pts[i]);
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            sink->Release();
        }

        if (m_bodyGradient) {
            m_bodyGradient->SetTransform(transform);
            rt->FillGeometry(bodyGeo, m_bodyGradient);
        } else {
            ID2D1SolidColorBrush* brush = nullptr;
            if (SUCCEEDED(rt->CreateSolidColorBrush(m_color.body, &brush))) {
                rt->FillGeometry(bodyGeo, brush);
                brush->Release();
            }
        }
    }

    // ── Tail fin ──
    if (m_hasStripe) {
        ID2D1SolidColorBrush* stripeBrush = nullptr;
        D2D1_COLOR_F stripeColor =
            (m_type == FishType::Torpedo)
                ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.55f)
                : D2D1::ColorF(m_color.accent.r, m_color.accent.g, m_color.accent.b, 0.72f);
        if (SUCCEEDED(rt->CreateSolidColorBrush(stripeColor, &stripeBrush)) && stripeBrush) {
            if (m_type == FishType::Torpedo) {
                rt->FillRectangle(D2D1::RectF(-35.0f, -3.0f, 35.0f, 2.0f), stripeBrush);
            } else {
                float stripeW = (m_type == FishType::Flat) ? 7.0f : 6.0f;
                rt->FillRectangle(D2D1::RectF(-24.0f, -24.0f, -24.0f + stripeW, 24.0f), stripeBrush);
                rt->FillRectangle(D2D1::RectF(2.0f, -22.0f, 2.0f + stripeW, 22.0f), stripeBrush);
                if (m_type == FishType::Round) {
                    rt->FillRectangle(D2D1::RectF(26.0f, -16.0f, 26.0f + stripeW, 16.0f), stripeBrush);
                }
            }
            stripeBrush->Release();
        }
    }

    ID2D1PathGeometry* tailGeo = nullptr;
    hr = factory->CreatePathGeometry(&tailGeo);
    if (SUCCEEDED(hr) && tailGeo) {
        ID2D1GeometrySink* sink = nullptr;
        hr = tailGeo->Open(&sink);
        if (SUCCEEDED(hr) && sink) {
            BuildTailGeometry(sink, shape, m_tailPhase, halfLen);
            sink->Close();
            sink->Release();

            ID2D1SolidColorBrush* brush = nullptr;
            rt->CreateSolidColorBrush(m_color.fin, &brush);
            if (brush) {
                rt->FillGeometry(tailGeo, brush);
                brush->Release();
            }
        }
        tailGeo->Release();
    }

    // ── Dorsal fin (animated) ──
    float finFlap = sinf(m_tailPhase) * 3.5f;
    float dTop = (m_type == FishType::Flat) ? -30.0f : -20.0f;
    ID2D1PathGeometry* finGeo = nullptr;
    hr = factory->CreatePathGeometry(&finGeo);
    if (SUCCEEDED(hr) && finGeo) {
        ID2D1GeometrySink* sink = nullptr;
        hr = finGeo->Open(&sink);
        if (SUCCEEDED(hr) && sink) {
            float cx = (m_type == FishType::Torpedo) ? -15.0f : -12.0f;
            sink->BeginFigure(D2D1::Point2F(cx - 8, dTop), D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine(D2D1::Point2F(cx + finFlap, dTop - 12));
            sink->AddLine(D2D1::Point2F(cx + 8, dTop));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            sink->Release();

            ID2D1SolidColorBrush* brush = nullptr;
            rt->CreateSolidColorBrush(Brighten(m_color.body, 1.2f), &brush);
            if (brush) {
                rt->FillGeometry(finGeo, brush);
                brush->Release();
            }
        }
        finGeo->Release();
    }

    // ── Pectoral fin (animated) ──
    float pectFlap = cosf(m_tailPhase + 0.8f) * 4.0f;
    ID2D1PathGeometry* pectGeo = nullptr;
    hr = factory->CreatePathGeometry(&pectGeo);
    if (SUCCEEDED(hr) && pectGeo) {
        ID2D1GeometrySink* sink = nullptr;
        hr = pectGeo->Open(&sink);
        if (SUCCEEDED(hr) && sink) {
            float px = halfLen * 0.15f;
            float py = (m_type == FishType::Flat) ? 18.0f : 14.0f;
            sink->BeginFigure(D2D1::Point2F(px - 4, py), D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine(D2D1::Point2F(px - 12 + pectFlap, py + 14));
            sink->AddLine(D2D1::Point2F(px + 4, py + 6));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            sink->Release();

            ID2D1SolidColorBrush* brush = nullptr;
            rt->CreateSolidColorBrush(Brighten(m_color.body, 1.15f), &brush);
            if (brush) {
                rt->FillGeometry(pectGeo, brush);
                brush->Release();
            }
        }
        pectGeo->Release();
    }

    // ── Eye ──
    ID2D1EllipseGeometry* eyeGeo = nullptr;
    hr = factory->CreateEllipseGeometry(
        D2D1::Ellipse(D2D1::Point2F(shape->eyeX, shape->eyeY), shape->eyeR, shape->eyeR),
        &eyeGeo);
    if (SUCCEEDED(hr) && eyeGeo) {
        ID2D1SolidColorBrush* brush = nullptr;
        rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush);
        if (brush) {
            rt->FillGeometry(eyeGeo, brush);
            brush->Release();
        }
        // Pupil
        ID2D1EllipseGeometry* pupil = nullptr;
        float pr = shape->eyeR * 0.5f;
        hr = factory->CreateEllipseGeometry(
            D2D1::Ellipse(D2D1::Point2F(shape->eyeX - 1.5f, shape->eyeY), pr, pr),
            &pupil);
        if (SUCCEEDED(hr) && pupil) {
            ID2D1SolidColorBrush* brush = nullptr;
            rt->CreateSolidColorBrush(m_color.accent, &brush);
            if (brush) {
                rt->FillGeometry(pupil, brush);
                brush->Release();
            }
            pupil->Release();
        }
        // Eye catchlight
        float hlR = shape->eyeR * 0.22f;
        ID2D1EllipseGeometry* hlGeo = nullptr;
        hr = factory->CreateEllipseGeometry(
            D2D1::Ellipse(D2D1::Point2F(shape->eyeX - 2.0f, shape->eyeY - 1.5f), hlR, hlR),
            &hlGeo);
        if (SUCCEEDED(hr) && hlGeo) {
            ID2D1SolidColorBrush* brush = nullptr;
            rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush);
            if (brush) {
                rt->FillGeometry(hlGeo, brush);
                brush->Release();
            }
            hlGeo->Release();
        }
        eyeGeo->Release();
    }

    // ── Mouth (animated: opens/closes with breathing rhythm) ──
    float mouthOpen = 0.5f + (sinf(m_tailPhase * 0.7f) + 1.0f) * 0.65f;
    ID2D1EllipseGeometry* mouthGeo = nullptr;
    hr = factory->CreateEllipseGeometry(
        D2D1::Ellipse(D2D1::Point2F(halfLen - 2, -1.5f), mouthOpen * 2.0f, 1.5f),
        &mouthGeo);
    if (SUCCEEDED(hr) && mouthGeo) {
        ID2D1SolidColorBrush* brush = nullptr;
        rt->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.1f, 0.1f, 1.0f), &brush);
        if (brush) {
            rt->FillGeometry(mouthGeo, brush);
            brush->Release();
        }
        mouthGeo->Release();
    }

    // ── Cleanup ──
    if (bodyGeo) bodyGeo->Release();
    rt->SetTransform(D2D1::Matrix3x2F::Identity());
    factory->Release();
}

void Fish::ApplySteering(Vec2 force, float dt) {
    force.Truncate(MAX_FISH_FORCE * (1.0f + m_sprintBoost * 1.4f));
    m_velocity += force * dt;
    m_velocity.Truncate(MAX_FISH_SPEED * (1.0f + m_sprintBoost));

    if (m_velocity.Length() < 20.0f) {
        m_velocity = m_velocity.Normalized() * 20.0f;
    }
}

Vec2 Fish::Separation(const BoidsContext& ctx) const {
    Vec2 steer;
    int count = 0;
    for (int i = 0; i < ctx.count; ++i) {
        if (i == ctx.selfIndex) continue;
        Vec2 diff = m_position - ctx.positions[i];
        float d = diff.Length();
        if (d > 0 && d < SEPARATION_RADIUS) {
            steer += diff / (d * d);
            ++count;
        }
    }
    if (count > 0) {
        steer = steer / (float)count;
        steer = steer.Normalized() * MAX_FISH_SPEED - m_velocity;
    }
    return steer;
}

Vec2 Fish::Alignment(const BoidsContext& ctx) const {
    Vec2 avgVel;
    int count = 0;
    for (int i = 0; i < ctx.count; ++i) {
        if (i == ctx.selfIndex) continue;
        if ((ctx.positions[i] - m_position).Length() < ALIGNMENT_RADIUS) {
            avgVel += ctx.velocities[i];
            ++count;
        }
    }
    if (count > 0) {
        avgVel = avgVel / (float)count;
        return avgVel.Normalized() * MAX_FISH_SPEED - m_velocity;
    }
    return {};
}

Vec2 Fish::Cohesion(const BoidsContext& ctx) const {
    Vec2 center;
    int count = 0;
    for (int i = 0; i < ctx.count; ++i) {
        if (i == ctx.selfIndex) continue;
        if ((ctx.positions[i] - m_position).Length() < COHESION_RADIUS) {
            center += ctx.positions[i];
            ++count;
        }
    }
    if (count > 0) {
        center = center / (float)count;
        return (center - m_position).Normalized() * MAX_FISH_SPEED - m_velocity;
    }
    return {};
}

Vec2 Fish::SeekFood(const BoidsContext& ctx) const {
    if (ctx.foodCount <= 0 || !ctx.foodPositions) return {};

    Vec2 target;
    float closestDist = ctx.foodDetectRadius;
    for (int i = 0; i < ctx.foodCount; ++i) {
        float d = (ctx.foodPositions[i] - m_position).Length();
        if (d < closestDist) {
            closestDist = d;
            target = ctx.foodPositions[i];
        }
    }

    if (closestDist >= ctx.foodDetectRadius) return {};

    float urgency = 1.0f - closestDist / ctx.foodDetectRadius;
    bool interested = m_foodInterest > 0.42f || closestDist < ctx.foodDetectRadius * 0.32f;
    if (!interested) return {};

    float desiredSpeed = MAX_FISH_SPEED * (1.15f + urgency * (0.75f + m_foodInterest));
    return (target - m_position).Normalized() * desiredSpeed - m_velocity;
}

ID2D1Factory* Fish::GetFactory(ID2D1DCRenderTarget* rt) {
    ID2D1Factory* factory = nullptr;
    rt->GetFactory(&factory);
    return factory;
}
