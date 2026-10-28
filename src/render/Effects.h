#pragma once

#include "core/Random.h"
#include "render/Theme.h"

#include <raylib.h>

#include <string>
#include <vector>

/// @file Effects.h
/// Short-lived visual feedback: particle bursts and toast messages.

namespace render {

/// One glowing particle in screen space.
struct Particle {
    Vector2 position{};
    Vector2 velocity{};
    float age = 0.0f;
    float lifetime = 1.0f;
    float size = 3.0f;
    Color color{};
};

/// A message that slides in at the top of the play area and fades out.
struct Toast {
    std::string text;
    Color color{};
    float age = 0.0f;
};

/// Owns and animates particles and toasts. Everything is cosmetic; game rules never depend
/// on it.
class Effects {
public:
    /// Creates an empty effect pool.
    Effects();

    /// Spawns @p count particles flying outwards from @p position.
    void burst(Vector2 position, Color color, int count, float speed);

    /// Spawns particles on a ring (used for the phase activation).
    void ring(Vector2 position, Color color, float radius, int count);

    /// Shows a toast message.
    void toast(std::string text, Color color);

    /// Advances all particles and toasts.
    void update(float deltaSeconds);

    /// Removes everything (new level).
    void clear();

    /// Draws the particles.
    void drawParticles() const;

    /// Draws the toasts stacked below @p topY, centred horizontally.
    void drawToasts(const Theme& theme, float topY) const;

private:
    std::vector<Particle> m_particles;
    std::vector<Toast> m_toasts;
    core::Rng m_rng;
};

} // namespace render
