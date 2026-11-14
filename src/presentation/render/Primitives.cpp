#include "render/Primitives.h"

#include <rlgl.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace render {

namespace {

constexpr float TWO_PI = 6.28318530718f;
constexpr float HALF_PI = 1.57079632679f;
constexpr int STAR_POINTS = 5;
constexpr float STAR_INNER_RATIO = 0.45f;
constexpr float BACK_OVERSHOOT = 1.70158f;
constexpr int ROUNDED_SEGMENTS = 8;

/// The ten outline vertices of a star, starting at the top point.
std::array<Vector2, 2 * STAR_POINTS> starVertices(Vector2 center, float radius,
                                                  float rotationDegrees) {
    std::array<Vector2, 2 * STAR_POINTS> vertices{};
    const float rotation = rotationDegrees * DEG2RAD - HALF_PI;
    for (std::size_t index = 0; index < vertices.size(); ++index) {
        const float angle =
            rotation + static_cast<float>(index) * TWO_PI / static_cast<float>(vertices.size());
        const float length = (index % 2 == 0) ? radius : radius * STAR_INNER_RATIO;
        vertices[index] =
            Vector2{center.x + std::cos(angle) * length, center.y + std::sin(angle) * length};
    }
    return vertices;
}

unsigned char lerpChannel(unsigned char from, unsigned char to, float amount) {
    const float value = static_cast<float>(from) + (static_cast<float>(to) - from) * amount;
    return static_cast<unsigned char>(std::clamp(value, 0.0f, 255.0f));
}

} // namespace

float easeOutCubic(float t) {
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    const float inverse = 1.0f - clamped;
    return 1.0f - inverse * inverse * inverse;
}

float easeInOutSine(float t) {
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    return 0.5f - 0.5f * std::cos(clamped * PI);
}

float easeOutBack(float t) {
    const float clamped = std::clamp(t, 0.0f, 1.0f) - 1.0f;
    return 1.0f + (BACK_OVERSHOOT + 1.0f) * clamped * clamped * clamped +
           BACK_OVERSHOOT * clamped * clamped;
}

float pulse(float timeSeconds, float frequency) {
    return 0.5f + 0.5f * std::sin(timeSeconds * frequency * TWO_PI);
}

Color fade(Color color, float factor) {
    const float alpha = static_cast<float>(color.a) * std::clamp(factor, 0.0f, 1.0f);
    color.a = static_cast<unsigned char>(alpha);
    return color;
}

Color mix(Color from, Color to, float amount) {
    const float t = std::clamp(amount, 0.0f, 1.0f);
    return Color{lerpChannel(from.r, to.r, t), lerpChannel(from.g, to.g, t),
                 lerpChannel(from.b, to.b, t), lerpChannel(from.a, to.a, t)};
}

void drawTriangleAnyWinding(Vector2 a, Vector2 b, Vector2 c, Color color) {
    // raylib expects counter-clockwise order on screen, which has a negative cross product.
    const float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cross > 0.0f) {
        DrawTriangle(a, c, b, color);
    } else {
        DrawTriangle(a, b, c, color);
    }
}

void drawStar(Vector2 center, float radius, float rotationDegrees, Color color) {
    const auto vertices = starVertices(center, radius, rotationDegrees);
    for (std::size_t index = 0; index < vertices.size(); ++index) {
        const Vector2 next = vertices[(index + 1) % vertices.size()];
        drawTriangleAnyWinding(center, vertices[index], next, color);
    }
}

void drawStarOutline(Vector2 center, float radius, float thickness, Color color) {
    const auto vertices = starVertices(center, radius, 0.0f);
    for (std::size_t index = 0; index < vertices.size(); ++index) {
        DrawLineEx(vertices[index], vertices[(index + 1) % vertices.size()], thickness, color);
    }
}

void drawGlow(Vector2 center, float radius, Color color) {
    Color transparent = color;
    transparent.a = 0;
    DrawCircleGradient(center, radius, color, transparent);
}

void drawLockIcon(Vector2 center, float size, Color color) {
    const float bodyWidth = size * 0.7f;
    const float bodyHeight = size * 0.5f;
    const Rectangle body{center.x - bodyWidth / 2.0f, center.y - bodyHeight * 0.2f, bodyWidth,
                         bodyHeight};
    const float shackleRadius = bodyWidth * 0.32f;
    const Vector2 shackleCenter{center.x, body.y};
    DrawRing(shackleCenter, shackleRadius * 0.62f, shackleRadius, 180.0f, 360.0f,
             ROUNDED_SEGMENTS * 2, color);
    DrawRectangleRounded(body, 0.25f, ROUNDED_SEGMENTS, color);
}

void beginPremultipliedAccumulation() {
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA,
                              RL_FUNC_ADD, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
}

void drawPanel(Rectangle bounds, float cornerRadius, Color fill, Color border,
               float borderThickness) {
    const float shortest = std::max(1.0f, std::min(bounds.width, bounds.height));
    const float roundness = std::clamp(2.0f * cornerRadius / shortest, 0.0f, 1.0f);
    DrawRectangleRounded(bounds, roundness, ROUNDED_SEGMENTS, fill);
    if (borderThickness > 0.0f && border.a > 0) {
        DrawRectangleRoundedLinesEx(bounds, roundness, ROUNDED_SEGMENTS, borderThickness, border);
    }
}

} // namespace render
