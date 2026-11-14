#include "render/Minimap.h"

#include "render/Hud.h"
#include "render/Palette.h"
#include "render/PerkIcons.h"
#include "render/Primitives.h"
#include "render/SkinRenderer.h"

#include <algorithm>
#include <cmath>

namespace render {

namespace {

constexpr float SMALL_SIZE = 200.0f;
constexpr float LARGE_SIZE = 400.0f;
constexpr float EDGE_MARGIN = 14.0f;
constexpr float INNER_PADDING = 8.0f;

/// Wall segment of @p cell on @p side, in map pixels.
void drawWallSegment(core::Cell cell, core::Direction side, float cellPixels, float thickness) {
    const float x = static_cast<float>(cell.x) * cellPixels;
    const float y = static_cast<float>(cell.y) * cellPixels;
    Vector2 from{x, y};
    Vector2 to{x + cellPixels, y};
    if (side == core::Direction::South) {
        from = Vector2{x, y + cellPixels};
        to = Vector2{x + cellPixels, y + cellPixels};
    } else if (side == core::Direction::West) {
        to = Vector2{x, y + cellPixels};
    } else if (side == core::Direction::East) {
        from = Vector2{x + cellPixels, y};
        to = Vector2{x + cellPixels, y + cellPixels};
    }
    DrawLineEx(from, to, thickness, palette::WALL_GLOW);
}

} // namespace

void Minimap::reset() {
    m_cache.reset();
    m_cachedExplored = -1;
}

Rectangle Minimap::bounds(const Theme& theme) const {
    const float size = theme.px(m_large ? LARGE_SIZE : SMALL_SIZE);
    const float margin = theme.px(EDGE_MARGIN);
    return Rectangle{static_cast<float>(GetScreenWidth()) - size - margin,
                     Hud::topBarHeight(theme) + margin, size, size};
}

void Minimap::rebuildCache(const core::Level& level, int pixels) {
    if (!m_cache.isValid() || m_cache.width() != pixels) {
        m_cache = RenderTarget(pixels, pixels);
    }
    const core::Maze& maze = level.maze();
    const float cellPixels = static_cast<float>(pixels) / static_cast<float>(maze.size());
    const float thickness = std::max(1.0f, cellPixels * 0.18f);
    BeginTextureMode(m_cache.get());
    ClearBackground(BLANK);
    beginPremultipliedAccumulation();
    for (int y = 0; y < maze.size(); ++y) {
        for (int x = 0; x < maze.size(); ++x) {
            const core::Cell cell{x, y};
            if (!level.fog().isExplored(cell)) {
                continue;
            }
            DrawRectangleRec(Rectangle{static_cast<float>(x) * cellPixels,
                                       static_cast<float>(y) * cellPixels, cellPixels, cellPixels},
                             fade(palette::PANEL_HI, 0.75f));
            for (const core::Direction side : core::ALL_DIRECTIONS) {
                if (maze.hasWall(cell, side)) {
                    drawWallSegment(cell, side, cellPixels, thickness);
                }
            }
        }
    }
    EndBlendMode();
    EndTextureMode();
    m_cachedExplored = level.fog().exploredCount();
    m_cachedPixels = pixels;
}

Vector2 Minimap::cellCenter(Rectangle area, int mazeSize, core::Vec2f cell) const {
    const float cellPixels = area.width / static_cast<float>(mazeSize);
    return Vector2{area.x + (cell.x + 0.5f) * cellPixels, area.y + (cell.y + 0.5f) * cellPixels};
}

void Minimap::drawMarkers(Rectangle area, const core::Level& level, float time) const {
    const int size = level.maze().size();
    const float cellPixels = area.width / static_cast<float>(size);
    const float dot = std::max(2.5f, cellPixels * 0.32f);
    const core::Cell exit = level.maze().exit();
    if (level.fog().isExplored(exit)) {
        const Vector2 center = cellCenter(
            area, size, core::Vec2f{static_cast<float>(exit.x), static_cast<float>(exit.y)});
        drawGlow(center, dot * 3.0f, fade(palette::ACCENT_GREEN, 0.5f + 0.3f * pulse(time, 1.0f)));
        DrawCircleV(center, dot, palette::ACCENT_GREEN);
    }
    for (const core::PerkItem& perk : level.perks()) {
        if (!perk.collected && level.fog().isExplored(perk.cell)) {
            DrawCircleV(cellCenter(area, size,
                                   core::Vec2f{static_cast<float>(perk.cell.x),
                                               static_cast<float>(perk.cell.y)}),
                        dot * 0.8f, perkColor(perk.type));
        }
    }
    const core::Chaser& chaser = level.chaser();
    const bool chaserSeen = !level.spec().hasFog || level.fog().isVisible(chaser.cell());
    if (chaser.isActive() && chaserSeen) {
        const Vector2 center = cellCenter(area, size, chaser.visualPosition());
        drawGlow(center, dot * 3.0f, fade(palette::DANGER, 0.6f));
        DrawCircleV(center, dot, palette::DANGER);
    }
}

void Minimap::drawPlayerArrow(Rectangle area, const core::Level& level, float yaw,
                              Color color) const {
    const int size = level.maze().size();
    const float cellPixels = area.width / static_cast<float>(size);
    const float length = std::max(6.0f, cellPixels * 0.9f);
    const Vector2 center = cellCenter(area, size, level.player().visualPosition());
    // yaw 0 points north (up on the map) and grows clockwise.
    const Vector2 forward{std::sin(yaw), -std::cos(yaw)};
    const Vector2 side{-forward.y, forward.x};
    const Vector2 tip{center.x + forward.x * length * 0.6f, center.y + forward.y * length * 0.6f};
    const Vector2 left{center.x - forward.x * length * 0.4f + side.x * length * 0.38f,
                       center.y - forward.y * length * 0.4f + side.y * length * 0.38f};
    const Vector2 right{center.x - forward.x * length * 0.4f - side.x * length * 0.38f,
                        center.y - forward.y * length * 0.4f - side.y * length * 0.38f};
    drawGlow(center, length * 1.4f, fade(color, 0.5f));
    drawTriangleAnyWinding(tip, left, right, color);
}

void Minimap::draw(const Theme& theme, const core::Level& level, float yaw, core::SkinId skin,
                   float time) {
    const Rectangle frame = bounds(theme);
    const float padding = theme.px(INNER_PADDING);
    const Rectangle area{frame.x + padding, frame.y + padding, frame.width - 2.0f * padding,
                         frame.height - 2.0f * padding};
    const int pixels = static_cast<int>(area.width);
    if (level.fog().exploredCount() != m_cachedExplored || pixels != m_cachedPixels) {
        rebuildCache(level, pixels);
    }
    drawPanel(frame, theme.px(10.0f), fade(palette::BG_DEEP, 0.88f), palette::WALL, theme.px(1.5f));
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    DrawTextureRec(m_cache.get().texture,
                   Rectangle{0.0f, 0.0f, static_cast<float>(pixels), -static_cast<float>(pixels)},
                   Vector2{area.x, area.y}, WHITE);
    EndBlendMode();
    drawMarkers(area, level, time);
    drawPlayerArrow(area, level, yaw, skinColor(skin));
    theme.drawText(m_large ? "M  smaller map" : "M  larger map",
                   Vector2{frame.x, frame.y + frame.height + theme.px(4.0f)},
                   theme.px(textsize::TINY), palette::TEXT_MUTED);
}

} // namespace render
