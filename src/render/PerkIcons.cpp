#include "render/PerkIcons.h"

#include "render/Palette.h"
#include "render/Primitives.h"

#include <cmath>

namespace render {

namespace {

constexpr float SPIN_DEGREES_PER_SECOND = 70.0f;

void drawHourglass(Vector2 center, float radius, float time) {
    const float half = radius * 0.62f;
    const float tilt = 0.08f * std::sin(time * 2.0f);
    const Vector2 top{center.x + tilt * radius, center.y - radius * 0.82f};
    const Vector2 bottom{center.x - tilt * radius, center.y + radius * 0.82f};
    drawTriangleAnyWinding(Vector2{top.x - half, top.y}, Vector2{top.x + half, top.y}, center,
                           palette::ACCENT_BLUE);
    drawTriangleAnyWinding(Vector2{bottom.x - half, bottom.y}, Vector2{bottom.x + half, bottom.y},
                           center, fade(palette::ACCENT_BLUE, 0.75f));
    const float bar = radius * 0.12f;
    DrawRectangleRec(Rectangle{top.x - half * 1.2f, top.y - bar, half * 2.4f, bar}, palette::TEXT);
    DrawRectangleRec(Rectangle{bottom.x - half * 1.2f, bottom.y, half * 2.4f, bar}, palette::TEXT);
    DrawCircleV(Vector2{center.x, center.y + radius * 0.45f}, radius * 0.16f,
                palette::ACCENT_GREEN);
}

void drawCompassSpark(Vector2 center, float radius, float time) {
    const float spin = time * SPIN_DEGREES_PER_SECOND * DEG2RAD;
    for (int point = 0; point < 4; ++point) {
        const float angle = spin + static_cast<float>(point) * PI / 2.0f;
        const Vector2 tip{center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius};
        const float side = angle + PI / 2.0f;
        const float width = radius * 0.22f;
        const Vector2 left{center.x + std::cos(side) * width, center.y + std::sin(side) * width};
        const Vector2 right{center.x - std::cos(side) * width, center.y - std::sin(side) * width};
        drawTriangleAnyWinding(tip, left, right, palette::ACCENT_GREEN);
    }
    DrawCircleV(center, radius * 0.2f, palette::TEXT);
}

void drawLantern(Vector2 center, float radius, float time) {
    const float flicker = 0.8f + 0.2f * pulse(time, 3.1f);
    const Rectangle body{center.x - radius * 0.45f, center.y - radius * 0.5f, radius * 0.9f,
                         radius * 1.15f};
    DrawRing(Vector2{center.x, body.y}, radius * 0.25f, radius * 0.36f, 180.0f, 360.0f, 12,
             palette::TEXT_MUTED);
    DrawRectangleRounded(body, 0.35f, 6, palette::ACCENT_GREEN_DIM);
    drawGlow(Vector2{center.x, center.y + radius * 0.08f}, radius * 0.9f * flicker,
             fade(palette::ACCENT_GREEN, 0.8f));
    DrawCircleV(Vector2{center.x, center.y + radius * 0.08f}, radius * 0.2f * flicker,
                palette::TEXT);
}

void drawFrostShard(Vector2 center, float radius, float time) {
    const float sway = 0.06f * radius * std::sin(time * 1.7f);
    const Vector2 top{center.x + sway, center.y - radius};
    const Vector2 bottom{center.x - sway, center.y + radius};
    const Vector2 left{center.x - radius * 0.45f, center.y + radius * 0.1f};
    const Vector2 right{center.x + radius * 0.45f, center.y - radius * 0.1f};
    drawTriangleAnyWinding(top, left, bottom, palette::ACCENT_BLUE);
    drawTriangleAnyWinding(top, right, bottom, mix(palette::ACCENT_BLUE, palette::TEXT, 0.45f));
    DrawLineEx(top, bottom, radius * 0.06f, fade(palette::TEXT, 0.8f));
}

} // namespace

Color perkColor(core::PerkType type) {
    switch (type) {
        case core::PerkType::MagicBerry:
            return palette::WALL_GLOW;
        case core::PerkType::Hourglass:
        case core::PerkType::FrostShard:
            return palette::ACCENT_BLUE;
        case core::PerkType::CompassSpark:
        case core::PerkType::Lantern:
            return palette::ACCENT_GREEN;
    }
    return palette::TEXT;
}

Color effectColor(core::EffectType type) {
    switch (type) {
        case core::EffectType::Phase:
            return palette::WALL_GLOW;
        case core::EffectType::TimeFreeze:
        case core::EffectType::ChaserFreeze:
            return palette::ACCENT_BLUE;
        case core::EffectType::CompassTrail:
        case core::EffectType::Lantern:
            return palette::ACCENT_GREEN;
    }
    return palette::TEXT;
}

void drawBerryIcon(Vector2 center, float radius, bool filled, float time) {
    if (!filled) {
        DrawCircleLinesV(center, radius * 0.9f, fade(palette::TEXT_MUTED, 0.5f));
        return;
    }
    const float bob = 0.06f * radius * pulse(time, 1.0f);
    const float berry = radius * 0.48f;
    drawGlow(center, radius * 1.6f, fade(palette::WALL_GLOW, 0.35f));
    DrawCircleV(Vector2{center.x - berry * 0.75f, center.y + berry * 0.35f + bob}, berry,
                palette::WALL_GLOW);
    DrawCircleV(Vector2{center.x + berry * 0.75f, center.y + berry * 0.35f + bob}, berry,
                palette::WALL);
    DrawCircleV(Vector2{center.x, center.y - berry * 0.45f + bob}, berry, palette::WALL_GLOW);
    DrawCircleV(Vector2{center.x - berry * 0.3f, center.y - berry * 0.7f + bob}, berry * 0.28f,
                palette::TEXT);
    drawTriangleAnyWinding(Vector2{center.x, center.y - radius * 0.95f},
                           Vector2{center.x + radius * 0.55f, center.y - radius * 1.15f},
                           Vector2{center.x + radius * 0.2f, center.y - radius * 0.6f},
                           palette::ACCENT_GREEN);
}

void drawPerkIcon(core::PerkType type, Vector2 center, float radius, float time) {
    drawGlow(center, radius * 1.7f, fade(perkColor(type), 0.30f + 0.15f * pulse(time, 1.2f)));
    switch (type) {
        case core::PerkType::MagicBerry:
            drawBerryIcon(center, radius, true, time);
            break;
        case core::PerkType::Hourglass:
            drawHourglass(center, radius, time);
            break;
        case core::PerkType::CompassSpark:
            drawCompassSpark(center, radius, time);
            break;
        case core::PerkType::Lantern:
            drawLantern(center, radius, time);
            break;
        case core::PerkType::FrostShard:
            drawFrostShard(center, radius, time);
            break;
    }
}

} // namespace render
