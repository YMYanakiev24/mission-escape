#include "render/SkinRenderer.h"

#include "render/Palette.h"
#include "render/Primitives.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace render {

namespace {

constexpr float TWO_PI = 6.28318530718f;
constexpr std::size_t MAX_TRAIL_POINTS = 16;
constexpr int IDLE_FRAMES_PER_SHRINK = 2;
constexpr int WISP_SPARKS = 3;
constexpr int NOVA_RAYS = 8;
constexpr int PHANTOM_WAVES = 3;
constexpr int SYNTHETIC_TRAIL_POINTS = 7;

Vector2 offset(Vector2 origin, float angle, float distance) {
    return Vector2{origin.x + std::cos(angle) * distance, origin.y + std::sin(angle) * distance};
}

void drawWisp(Vector2 center, float radius, float time) {
    drawGlow(center, radius * 2.6f, fade(palette::ACCENT_GREEN, 0.45f));
    DrawCircleV(center, radius * 0.95f, fade(palette::ACCENT_GREEN_DIM, 0.9f));
    DrawCircleV(center, radius * 0.78f, palette::ACCENT_GREEN);
    const float flicker = 0.36f + 0.08f * pulse(time, 2.3f) + 0.04f * pulse(time, 5.1f);
    DrawCircleV(center, radius * flicker, palette::TEXT);
    for (int spark = 0; spark < WISP_SPARKS; ++spark) {
        const float angle = time * 2.2f + static_cast<float>(spark) * TWO_PI / WISP_SPARKS;
        DrawCircleV(offset(center, angle, radius * 1.3f), radius * 0.13f,
                    fade(palette::ACCENT_GREEN, 0.9f));
    }
}

void drawPrism(Vector2 center, float radius, float time) {
    const float spin = time * 45.0f;
    const Color facet = mix(palette::ACCENT_BLUE, palette::WALL_GLOW, pulse(time, 0.7f));
    drawGlow(center, radius * 2.5f, fade(palette::WALL_GLOW, 0.45f));
    DrawPoly(center, 6, radius * 1.05f, spin, palette::WALL);
    DrawPoly(center, 3, radius * 0.85f, -spin * 1.4f, facet);
    DrawPoly(center, 3, radius * 0.85f, -spin * 1.4f + 60.0f, fade(palette::WALL_GLOW, 0.75f));
    DrawPolyLinesEx(center, 6, radius * 1.05f, spin, std::max(1.0f, radius * 0.1f),
                    palette::WALL_GLOW);
    DrawCircleV(center, radius * 0.22f, palette::TEXT);
}

void drawCometTail(const std::vector<Vector2>& points, float radius) {
    const std::size_t count = points.size();
    for (std::size_t index = 0; index < count; ++index) {
        const float age = static_cast<float>(index + 1) / static_cast<float>(count + 1);
        DrawCircleV(points[index], radius * (0.18f + 0.62f * age),
                    fade(palette::ACCENT_BLUE, 0.55f * age));
    }
}

std::vector<Vector2> syntheticTail(Vector2 center, float radius, float facing) {
    std::vector<Vector2> points;
    for (int index = SYNTHETIC_TRAIL_POINTS; index >= 1; --index) {
        points.push_back(offset(center, facing + PI, radius * 0.38f * static_cast<float>(index)));
    }
    return points;
}

void drawComet(Vector2 center, float radius, float time, float facing, const SkinTrail* trail) {
    drawCometTail(trail != nullptr ? trail->points() : syntheticTail(center, radius, facing),
                  radius);
    drawGlow(center, radius * 2.6f, fade(palette::ACCENT_BLUE, 0.55f));
    DrawCircleV(center, radius * 0.85f, mix(palette::ACCENT_BLUE, palette::TEXT, 0.35f));
    DrawCircleV(center, radius * (0.42f + 0.06f * pulse(time, 3.0f)), palette::TEXT);
    const Vector2 glint = offset(center, facing - PI / 4.0f, radius * 0.45f);
    DrawCircleV(glint, radius * 0.12f, palette::TEXT);
}

void drawPhantom(Vector2 center, float radius, float time, float facing) {
    const Color body = fade(mix(palette::WALL_GLOW, palette::TEXT, 0.45f), 0.88f);
    const float hover = std::sin(time * 2.4f) * radius * 0.08f;
    const Vector2 head{center.x, center.y - radius * 0.15f + hover};
    drawGlow(center, radius * 2.4f, fade(palette::WALL_GLOW, 0.40f));
    DrawCircleV(head, radius * 0.78f, body);
    DrawRectangleRec(Rectangle{head.x - radius * 0.78f, head.y, radius * 1.56f, radius * 0.62f},
                     body);
    // A rippling hem made of small scallops.
    const float hemY = head.y + radius * 0.62f;
    const float scallop = radius * 1.56f / static_cast<float>(PHANTOM_WAVES);
    for (int wave = 0; wave < PHANTOM_WAVES; ++wave) {
        const float x = head.x - radius * 0.78f + scallop * (static_cast<float>(wave) + 0.5f);
        const float ripple =
            std::sin(time * 6.0f + static_cast<float>(wave) * 1.9f) * radius * 0.07f;
        DrawCircleV(Vector2{x, hemY + ripple}, scallop * 0.5f, body);
    }
    // Eyes glance towards the direction of travel.
    const Vector2 look{std::cos(facing) * radius * 0.14f, std::sin(facing) * radius * 0.10f};
    for (const float side : {-1.0f, 1.0f}) {
        DrawEllipseV(Vector2{head.x + side * radius * 0.3f + look.x, head.y + look.y},
                     radius * 0.13f, radius * 0.2f, palette::BG_DEEP);
    }
}

void drawNova(Vector2 center, float radius, float time) {
    const float beat = pulse(time, 1.6f);
    drawGlow(center, radius * (2.6f + 0.5f * beat), fade(palette::ACCENT_GREEN, 0.5f));
    for (int ray = 0; ray < NOVA_RAYS; ++ray) {
        const float angle = time * 1.1f + static_cast<float>(ray) * TWO_PI / NOVA_RAYS;
        const float length = radius * ((ray % 2 == 0) ? 1.55f : 1.15f) * (0.9f + 0.15f * beat);
        const Color color = (ray % 2 == 0) ? palette::ACCENT_GREEN : palette::TEXT;
        drawTriangleAnyWinding(offset(center, angle, length),
                               offset(center, angle + 0.28f, radius * 0.45f),
                               offset(center, angle - 0.28f, radius * 0.45f), color);
    }
    DrawCircleV(center, radius * (0.62f + 0.08f * beat),
                mix(palette::ACCENT_GREEN, palette::TEXT, 0.5f));
    DrawCircleV(center, radius * 0.3f, palette::TEXT);
}

} // namespace

Color skinColor(core::SkinId skin) {
    switch (skin) {
        case core::SkinId::Wisp:
            return palette::ACCENT_GREEN;
        case core::SkinId::Prism:
            return palette::WALL_GLOW;
        case core::SkinId::Comet:
            return palette::ACCENT_BLUE;
        case core::SkinId::Phantom:
            return mix(palette::WALL_GLOW, palette::TEXT, 0.35f);
        case core::SkinId::Nova:
            return mix(palette::ACCENT_GREEN, palette::TEXT, 0.4f);
    }
    return palette::ACCENT_GREEN;
}

void SkinTrail::record(Vector2 position, float minimumSpacing) {
    const bool moved = m_points.empty() ||
                       std::hypot(position.x - m_points.back().x, position.y - m_points.back().y) >=
                           minimumSpacing;
    if (moved) {
        m_points.push_back(position);
        m_idleFrames = 0;
        if (m_points.size() > MAX_TRAIL_POINTS) {
            m_points.erase(m_points.begin());
        }
        return;
    }
    // Standing still: the tail retracts gradually instead of vanishing at once.
    if (++m_idleFrames >= IDLE_FRAMES_PER_SHRINK && m_points.size() > 1) {
        m_points.erase(m_points.begin());
        m_idleFrames = 0;
    }
}

void drawSkin(core::SkinId skin, Vector2 center, float radius, float time, float facingRadians,
              const SkinTrail* trail) {
    switch (skin) {
        case core::SkinId::Wisp:
            drawWisp(center, radius, time);
            break;
        case core::SkinId::Prism:
            drawPrism(center, radius, time);
            break;
        case core::SkinId::Comet:
            drawComet(center, radius, time, facingRadians, trail);
            break;
        case core::SkinId::Phantom:
            drawPhantom(center, radius, time, facingRadians);
            break;
        case core::SkinId::Nova:
            drawNova(center, radius, time);
            break;
    }
}

} // namespace render
