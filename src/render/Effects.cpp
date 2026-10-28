#include "render/Effects.h"

#include "render/Palette.h"
#include "render/Primitives.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace render {

namespace {

constexpr std::size_t MAX_PARTICLES = 600;
constexpr std::size_t MAX_TOASTS = 3;
constexpr float TOAST_SECONDS = 2.4f;
constexpr float TOAST_FADE_SECONDS = 0.35f;
constexpr float PARTICLE_DRAG = 2.2f;
constexpr float TWO_PI = 6.28318530718f;
constexpr std::uint32_t EFFECTS_SEED = 0xC0FFEEU;

} // namespace

Effects::Effects() : m_rng(EFFECTS_SEED) {}

void Effects::burst(Vector2 position, Color color, int count, float speed) {
    for (int index = 0; index < count && m_particles.size() < MAX_PARTICLES; ++index) {
        const float angle = m_rng.nextFloat() * TWO_PI;
        const float velocity = speed * (0.35f + 0.65f * m_rng.nextFloat());
        Particle particle;
        particle.position = position;
        particle.velocity = Vector2{std::cos(angle) * velocity, std::sin(angle) * velocity};
        particle.lifetime = 0.45f + 0.5f * m_rng.nextFloat();
        particle.size = 2.0f + 3.0f * m_rng.nextFloat();
        particle.color = m_rng.chance(0.25f) ? palette::TEXT : color;
        m_particles.push_back(particle);
    }
}

void Effects::ring(Vector2 position, Color color, float radius, int count) {
    for (int index = 0; index < count && m_particles.size() < MAX_PARTICLES; ++index) {
        const float angle = static_cast<float>(index) * TWO_PI / static_cast<float>(count);
        Particle particle;
        particle.position = Vector2{position.x + std::cos(angle) * radius * 0.3f,
                                    position.y + std::sin(angle) * radius * 0.3f};
        particle.velocity =
            Vector2{std::cos(angle) * radius * 2.2f, std::sin(angle) * radius * 2.2f};
        particle.lifetime = 0.55f;
        particle.size = 3.0f;
        particle.color = color;
        m_particles.push_back(particle);
    }
}

void Effects::toast(std::string text, Color color) {
    if (m_toasts.size() >= MAX_TOASTS) {
        m_toasts.erase(m_toasts.begin());
    }
    m_toasts.push_back(Toast{std::move(text), color, 0.0f});
}

void Effects::update(float deltaSeconds) {
    const float drag = std::exp(-PARTICLE_DRAG * deltaSeconds);
    for (Particle& particle : m_particles) {
        particle.age += deltaSeconds;
        particle.position.x += particle.velocity.x * deltaSeconds;
        particle.position.y += particle.velocity.y * deltaSeconds;
        particle.velocity.x *= drag;
        particle.velocity.y *= drag;
    }
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
                       [](const Particle& particle) { return particle.age >= particle.lifetime; }),
        m_particles.end());
    for (Toast& item : m_toasts) {
        item.age += deltaSeconds;
    }
    m_toasts.erase(std::remove_if(m_toasts.begin(), m_toasts.end(),
                                  [](const Toast& item) { return item.age >= TOAST_SECONDS; }),
                   m_toasts.end());
}

void Effects::clear() {
    m_particles.clear();
    m_toasts.clear();
}

void Effects::drawParticles() const {
    BeginBlendMode(BLEND_ADDITIVE);
    for (const Particle& particle : m_particles) {
        const float life = 1.0f - particle.age / particle.lifetime;
        DrawCircleV(particle.position, particle.size * (0.5f + 0.5f * life),
                    fade(particle.color, life));
    }
    EndBlendMode();
}

void Effects::drawToasts(const Theme& theme, float topY) const {
    float y = topY;
    const float size = theme.px(textsize::BODY);
    for (const Toast& item : m_toasts) {
        const float fadeIn = std::min(1.0f, item.age / TOAST_FADE_SECONDS);
        const float fadeOut = std::min(1.0f, (TOAST_SECONDS - item.age) / TOAST_FADE_SECONDS);
        const float alpha = std::clamp(std::min(fadeIn, fadeOut), 0.0f, 1.0f);
        const Vector2 extent = theme.measure(item.text, size);
        const float padding = theme.px(14.0f);
        const float slide = (1.0f - easeOutCubic(fadeIn)) * theme.px(12.0f);
        const Rectangle box{(static_cast<float>(GetScreenWidth()) - extent.x) / 2.0f - padding,
                            y - slide, extent.x + 2.0f * padding, extent.y + padding};
        drawPanel(box, theme.px(10.0f), fade(palette::PANEL_HI, 0.92f * alpha),
                  fade(item.color, alpha), theme.px(1.5f));
        theme.drawText(item.text, Vector2{box.x + padding, box.y + padding / 2.0f}, size,
                       fade(palette::TEXT, alpha));
        y += box.height + theme.px(8.0f);
    }
}

} // namespace render
