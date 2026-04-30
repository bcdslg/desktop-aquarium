#include "Food.h"
#include <cstdlib>
#include <cmath>

void FoodSystem::Spawn(float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        // Random scatter around click point
        float ox = (float)(rand() % 60 - 30);
        float oy = (float)(rand() % 20 - 10);
        p.position    = Vec2(x + ox, y + oy);
        p.drift       = Vec2((float)(rand() % 24 - 12) * 0.35f, 0.0f);
        p.lifetime    = FOOD_MAX_LIFETIME_MS / 1000.0f;
        p.wobblePhase = (float)(rand() % 628) / 100.0f;
        p.radius      = 2.2f + (float)(rand() % 22) / 10.0f;
        p.alpha       = 0.72f + (float)(rand() % 18) / 100.0f;
        int tone = rand() % 4;
        if (tone == 0)      p.color = RGBf(255, 196, 92, p.alpha);
        else if (tone == 1) p.color = RGBf(255, 154, 75, p.alpha);
        else if (tone == 2) p.color = RGBf(245, 213, 125, p.alpha);
        else                p.color = RGBf(230, 128, 72, p.alpha);
        p.alive       = true;
        m_particles.push_back(p);
    }
}

void FoodSystem::Update(float dt) {
    for (int i = (int)m_particles.size() - 1; i >= 0; --i) {
        auto& p = m_particles[i];
        if (!p.alive) {
            // Swap with last and pop (fast removal, order doesn't matter)
            if (i < (int)m_particles.size() - 1)
                p = m_particles.back();
            m_particles.pop_back();
            continue;
        }

        p.lifetime -= dt;
        if (p.lifetime <= 0) {
            if (i < (int)m_particles.size() - 1)
                p = m_particles.back();
            m_particles.pop_back();
            continue;
        }

        // Fall + wobble
        p.position.x += p.drift.x * dt;
        p.position.y += (FOOD_FALL_SPEED + p.radius * 5.0f) * dt;
        p.wobblePhase += dt * FOOD_WOBBLE_FREQ * 6.2832f;
    }
}

void FoodSystem::Draw(ID2D1DCRenderTarget* rt) const {
    if (m_particles.empty()) return;

    for (const auto& p : m_particles) {
        ID2D1SolidColorBrush* brush = nullptr;
        if (FAILED(rt->CreateSolidColorBrush(p.color, &brush)) || !brush) continue;

        float ageFade = (std::min)(1.0f, p.lifetime / 1.5f);
        brush->SetOpacity(p.alpha * ageFade);
        float wobble = sinf(p.wobblePhase) * (FOOD_WOBBLE_AMP * 0.55f);
        D2D1_ELLIPSE ellipse = {
            D2D1::Point2F(p.position.x + wobble, p.position.y),
            p.radius * 1.25f, p.radius
        };
        rt->FillEllipse(ellipse, brush);

        brush->SetColor(D2D1::ColorF(1.0f, 0.94f, 0.76f, 0.55f * ageFade));
        D2D1_ELLIPSE highlight = {
            D2D1::Point2F(p.position.x + wobble - p.radius * 0.35f,
                          p.position.y - p.radius * 0.28f),
            p.radius * 0.32f, p.radius * 0.24f
        };
        rt->FillEllipse(highlight, brush);
        brush->Release();
    }
}

void FoodSystem::Eat(int index) {
    if (index >= 0 && index < (int)m_particles.size()) {
        m_particles[index].alive = false;
    }
}
