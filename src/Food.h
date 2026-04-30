#pragma once
#include "Types.h"
#include <d2d1.h>
#include <vector>

class FoodSystem {
public:
    void Spawn(float x, float y, int count);
    void Update(float dt);
    void Draw(ID2D1DCRenderTarget* rt) const;

    // Access particles for fish collision
    struct Particle {
        Vec2  position;
        Vec2  drift;
        float lifetime;   // seconds remaining
        float wobblePhase;
        float radius;
        float alpha;
        D2D1_COLOR_F color;
        bool  alive;
    };

    const Particle* Particles() const { return m_particles.data(); }
    int             Count()     const { return (int)m_particles.size(); }

    void Eat(int index);  // remove particle at index

private:
    std::vector<Particle> m_particles;
};
