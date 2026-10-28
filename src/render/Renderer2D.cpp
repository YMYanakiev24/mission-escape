#include "render/Renderer2D.h"

#include "render/Palette.h"
#include "render/PerkIcons.h"
#include "render/Primitives.h"
#include "render/SkinRenderer.h"
#include "render/TierStyle.h"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace render {

namespace {

constexpr float FOLLOW_CELL_SIZE = 34.0f;
constexpr float MAX_FIT_CELL_SIZE = 72.0f;
constexpr float FIT_MARGIN = 0.96f;
constexpr float CAMERA_FOLLOW_RATE = 9.0f;
constexpr float CACHE_PADDING_CELLS = 0.5f;
constexpr float SIZE_EPSILON = 0.01f;

/// Glow layers rendered under the crisp wall line: (thickness in cells, opacity).
struct GlowLayer {
    float thickness;
    float opacity;
};
constexpr std::array<GlowLayer, 3> GLOW_LAYERS = {GlowLayer{0.46f, 0.07f}, GlowLayer{0.30f, 0.11f},
                                                  GlowLayer{0.18f, 0.20f}};
constexpr float WALL_THICKNESS = 0.10f;
constexpr float WALL_CORE_THICKNESS = 0.035f;
constexpr float MIN_WALL_PIXELS = 2.0f;

constexpr float EXIT_RADIUS = 0.42f;
constexpr float EXIT_SPIN_DEGREES_PER_SECOND = 90.0f;
constexpr float PLAYER_RADIUS = 0.28f;
constexpr float CHASER_RADIUS = 0.26f;
constexpr int CHASER_SPIKES = 8;
constexpr float FOG_FADE_RATE = 7.0f;
constexpr float FOG_UNKNOWN_ALPHA = 1.0f;
constexpr float FOG_MEMORY_ALPHA = 0.62f;
constexpr float FOG_LIGHT_FALLOFF = 0.35f;
constexpr float PLAYER_LIGHT_ALPHA = 0.16f;
constexpr float PHASE_WALL_OPACITY = 0.32f;
constexpr float PERK_RADIUS = 0.26f;
constexpr float TRAIL_SPACING = 0.12f;

/// Screen angle of a compass direction (0 = east, clockwise because y points down).
float directionAngle(core::Direction direction) {
    switch (direction) {
        case core::Direction::East:
            return 0.0f;
        case core::Direction::South:
            return PI / 2.0f;
        case core::Direction::West:
            return PI;
        case core::Direction::North:
            return -PI / 2.0f;
    }
    return 0.0f;
}
constexpr int RING_SEGMENTS = 24;

/// Full-texture source rectangle, flipped because render textures are stored upside down.
Rectangle flippedSource(const RenderTarget& target) {
    return Rectangle{0.0f, 0.0f, static_cast<float>(target.width()),
                     -static_cast<float>(target.height())};
}

} // namespace

void Renderer2D::beginLevel() {
    m_needsRebuild = true;
    m_cameraInitialised = false;
    m_trail.clear();
}

void Renderer2D::update(float deltaSeconds, const core::Level& level, Rectangle viewport,
                        float uiScale) {
    computeLayout(level.maze().size(), viewport, uiScale);
    if (m_needsRebuild || std::fabs(m_cachedCellSize - m_cellSize) > SIZE_EPSILON) {
        rebuildWallCache(level.maze());
    }
    updateCamera(deltaSeconds, level, !m_cameraInitialised);
    updateFog(deltaSeconds, level, !m_cameraInitialised);
    if (deltaSeconds > 0.0f) {
        m_trail.record(cellCenter(level.player().visualPosition()), m_cellSize * TRAIL_SPACING);
    }
    m_cameraInitialised = true;
}

void Renderer2D::computeLayout(int mazeSize, Rectangle viewport, float uiScale) {
    m_viewport = viewport;
    m_mazeSize = mazeSize;
    const float fit = std::floor(std::min(viewport.width, viewport.height) * FIT_MARGIN /
                                 static_cast<float>(std::max(1, mazeSize)));
    m_follow = fit < MIN_FIT_CELL_SIZE;
    m_cellSize = m_follow ? std::floor(std::max(MIN_FIT_CELL_SIZE, FOLLOW_CELL_SIZE * uiScale))
                          : std::min(fit, std::floor(MAX_FIT_CELL_SIZE * uiScale));
}

void Renderer2D::updateCamera(float deltaSeconds, const core::Level& level, bool snap) {
    const float mazePixels = static_cast<float>(level.maze().size()) * m_cellSize;
    Vector2 desired{mazePixels / 2.0f, mazePixels / 2.0f};
    if (m_follow) {
        const core::Vec2f player = level.player().visualPosition();
        desired = cellCenter(player);
        const float padding = m_cellSize * CACHE_PADDING_CELLS;
        const float halfWidth = m_viewport.width / 2.0f;
        const float halfHeight = m_viewport.height / 2.0f;
        // Keep the view inside the maze bounds unless the maze is smaller than the view.
        desired.x =
            (mazePixels + 2.0f * padding <= m_viewport.width)
                ? mazePixels / 2.0f
                : std::clamp(desired.x, halfWidth - padding, mazePixels + padding - halfWidth);
        desired.y =
            (mazePixels + 2.0f * padding <= m_viewport.height)
                ? mazePixels / 2.0f
                : std::clamp(desired.y, halfHeight - padding, mazePixels + padding - halfHeight);
    }
    const float blend = snap ? 1.0f : std::min(1.0f, deltaSeconds * CAMERA_FOLLOW_RATE);
    m_camera.target.x += (desired.x - m_camera.target.x) * blend;
    m_camera.target.y += (desired.y - m_camera.target.y) * blend;
    m_camera.offset =
        Vector2{m_viewport.x + m_viewport.width / 2.0f, m_viewport.y + m_viewport.height / 2.0f};
    m_camera.zoom = 1.0f;
    m_camera.rotation = 0.0f;
}

void Renderer2D::drawRuns(float thickness, Color color) const {
    const float padding = m_cellSize * CACHE_PADDING_CELLS;
    for (const WallRun& run : m_runs) {
        const Vector2 start{run.start.x * m_cellSize + padding, run.start.y * m_cellSize + padding};
        const Vector2 end{run.end.x * m_cellSize + padding, run.end.y * m_cellSize + padding};
        DrawLineEx(start, end, thickness, color);
        DrawCircleV(start, thickness / 2.0f, color); // rounded joints and caps
        DrawCircleV(end, thickness / 2.0f, color);
    }
}

void Renderer2D::rebuildWallCache(const core::Maze& maze) {
    m_runs = computeWallRuns(maze);
    const float padding = m_cellSize * CACHE_PADDING_CELLS;
    const int side =
        static_cast<int>(std::ceil(static_cast<float>(maze.size()) * m_cellSize + 2.0f * padding));
    m_wallCache = RenderTarget(side, side);
    m_scratch = RenderTarget(side, side);

    BeginTextureMode(m_wallCache.get());
    ClearBackground(BLANK);
    EndTextureMode();
    // Each glow layer is drawn opaque into a scratch target and then composited with a low
    // opacity, so overlapping runs never double-blend into bright blotches at the joints.
    for (const GlowLayer& layer : GLOW_LAYERS) {
        BeginTextureMode(m_scratch.get());
        ClearBackground(BLANK);
        drawRuns(std::max(MIN_WALL_PIXELS, layer.thickness * m_cellSize), palette::WALL_GLOW);
        EndTextureMode();
        BeginTextureMode(m_wallCache.get());
        beginPremultipliedAccumulation();
        DrawTextureRec(m_scratch.get().texture, flippedSource(m_scratch), Vector2{0.0f, 0.0f},
                       fade(WHITE, layer.opacity));
        EndBlendMode();
        EndTextureMode();
    }
    BeginTextureMode(m_wallCache.get());
    beginPremultipliedAccumulation();
    drawRuns(std::max(MIN_WALL_PIXELS, WALL_THICKNESS * m_cellSize), palette::WALL);
    drawRuns(std::max(1.0f, WALL_CORE_THICKNESS * m_cellSize),
             mix(palette::WALL_GLOW, palette::TEXT, 0.35f));
    EndBlendMode();
    EndTextureMode();
    m_scratch.reset();
    m_cachedCellSize = m_cellSize;
    m_needsRebuild = false;
}

Vector2 Renderer2D::cellCenter(core::Vec2f cellPosition) const {
    return Vector2{(cellPosition.x + 0.5f) * m_cellSize, (cellPosition.y + 0.5f) * m_cellSize};
}

Vector2 Renderer2D::toScreen(core::Vec2f cellPosition) const {
    return GetWorldToScreen2D(cellCenter(cellPosition), m_camera);
}

std::optional<core::Cell> Renderer2D::cellAt(Vector2 screenPosition) const {
    if (!CheckCollisionPointRec(screenPosition, m_viewport) || m_cellSize <= 0.0f) {
        return std::nullopt;
    }
    const Vector2 world = GetScreenToWorld2D(screenPosition, m_camera);
    const core::Cell cell{static_cast<int>(std::floor(world.x / m_cellSize)),
                          static_cast<int>(std::floor(world.y / m_cellSize))};
    if (cell.x < 0 || cell.y < 0 || cell.x >= m_mazeSize || cell.y >= m_mazeSize) {
        return std::nullopt;
    }
    return cell;
}

void Renderer2D::draw(const core::Level& level, float time) const {
    BeginScissorMode(static_cast<int>(m_viewport.x), static_cast<int>(m_viewport.y),
                     static_cast<int>(m_viewport.width), static_cast<int>(m_viewport.height));
    Camera2D camera = m_camera;
    camera.offset.x += m_shake.x;
    camera.offset.y += m_shake.y;
    BeginMode2D(camera);
    drawFloor(level);
    drawPlayerLight(level, time);
    drawWallCache(level);
    drawSecretDoors(level, time);
    drawExit(level.maze(), time);
    drawPerks(level, time);
    drawCompassTrail(level, time);
    drawChaser(level, time);
    drawFog(level);
    drawPlayer(level, time); // above the fog: the player is always in its own light
    drawExitHint(level, time);
    EndMode2D();
    EndScissorMode();
}

void Renderer2D::updateFog(float deltaSeconds, const core::Level& level, bool snap) {
    const int size = level.maze().size();
    if (m_fogAlpha.width() != size) {
        m_fogAlpha = core::Grid2D<float>(size, size, 1.0f);
        snap = true;
    }
    const core::Fog& fog = level.fog();
    const core::Vec2f player = level.player().visualPosition();
    const float radius = static_cast<float>(level.fogRadius()) + 0.5f;
    const float blend = snap ? 1.0f : std::min(1.0f, deltaSeconds * FOG_FADE_RATE);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float target = FOG_UNKNOWN_ALPHA;
            if (fog.isVisible(core::Cell{x, y})) {
                // Light falls off smoothly with distance, following the tweened player.
                const float distance =
                    std::hypot(static_cast<float>(x) - player.x, static_cast<float>(y) - player.y);
                target = FOG_LIGHT_FALLOFF * std::clamp((distance - 0.6f) / radius, 0.0f, 1.0f);
            } else if (fog.isExplored(core::Cell{x, y})) {
                target = FOG_MEMORY_ALPHA;
            }
            float& alpha = m_fogAlpha[y][x];
            alpha += (target - alpha) * blend;
        }
    }
}

void Renderer2D::drawFog(const core::Level& level) const {
    if (!level.spec().hasFog || m_fogAlpha.empty()) {
        return;
    }
    const Color fogColor = tierStyle(level.spec().tier).fog;
    const float overlap = 0.5f; // hides hairline seams between neighbouring fog cells
    for (int y = 0; y < m_fogAlpha.height(); ++y) {
        for (int x = 0; x < m_fogAlpha.width(); ++x) {
            const float alpha = m_fogAlpha[y][x];
            if (alpha < 0.01f) {
                continue;
            }
            DrawRectangleRec(Rectangle{static_cast<float>(x) * m_cellSize - overlap,
                                       static_cast<float>(y) * m_cellSize - overlap,
                                       m_cellSize + 2.0f * overlap, m_cellSize + 2.0f * overlap},
                             fade(fogColor, alpha));
        }
    }
}

void Renderer2D::drawPlayerLight(const core::Level& level, float time) const {
    if (!level.spec().hasFog || m_fogAlpha.empty()) {
        return;
    }
    // Cells in sight get a soft blue light that fades towards the edge of the sight radius.
    // It is drawn per cell, so it can never leak outside the maze or through the fog.
    const float flicker = 0.9f + 0.1f * pulse(time, 1.3f);
    for (int y = 0; y < m_fogAlpha.height(); ++y) {
        for (int x = 0; x < m_fogAlpha.width(); ++x) {
            const float light = 1.0f - m_fogAlpha[y][x] / FOG_LIGHT_FALLOFF;
            if (light <= 0.0f) {
                continue;
            }
            DrawRectangleRec(Rectangle{static_cast<float>(x) * m_cellSize,
                                       static_cast<float>(y) * m_cellSize, m_cellSize, m_cellSize},
                             fade(palette::ACCENT_BLUE, PLAYER_LIGHT_ALPHA * light * flicker));
        }
    }
}

void Renderer2D::drawExitHint(const core::Level& level, float time) const {
    if (!level.spec().hasFog || level.isExitSeen()) {
        return;
    }
    // A faint beacon through the fog gives a sense of direction until the exit is found.
    const core::Cell exit = level.maze().exit();
    const Vector2 center =
        cellCenter(core::Vec2f{static_cast<float>(exit.x), static_cast<float>(exit.y)});
    const float beat = pulse(time, 0.6f);
    drawGlow(center, m_cellSize * (1.6f + 0.5f * beat),
             fade(palette::ACCENT_GREEN, 0.10f + 0.10f * beat));
    DrawCircleV(center, m_cellSize * 0.08f, fade(palette::ACCENT_GREEN, 0.35f + 0.3f * beat));
}

void Renderer2D::drawChaser(const core::Level& level, float time) const {
    const core::Chaser& chaser = level.chaser();
    if (!chaser.isActive()) {
        return;
    }
    const Vector2 center = cellCenter(chaser.visualPosition());
    const float radius = m_cellSize * CHASER_RADIUS;
    const bool frozen = level.effects().isActive(core::EffectType::ChaserFreeze);
    const Color color =
        frozen ? mix(palette::DANGER, palette::ACCENT_BLUE, 0.75f) : palette::DANGER;
    const float motion = frozen ? 0.0f : time; // a frozen chaser stops bobbing and spinning
    const float bob = std::sin(motion * 6.0f) * radius * 0.08f;
    const Vector2 body{center.x, center.y + bob};
    drawGlow(body, radius * 2.6f, fade(color, 0.40f));
    if (frozen) {
        DrawPolyLinesEx(body, 6, radius * 1.8f, time * 20.0f, std::max(1.5f, radius * 0.12f),
                        fade(palette::ACCENT_BLUE, 0.8f));
    }
    const float spin = motion * 120.0f * DEG2RAD;
    for (int spike = 0; spike < CHASER_SPIKES; ++spike) {
        const float angle = spin + static_cast<float>(spike) * 2.0f * PI / CHASER_SPIKES;
        const Vector2 tip{body.x + std::cos(angle) * radius * 1.45f,
                          body.y + std::sin(angle) * radius * 1.45f};
        const Vector2 left{body.x + std::cos(angle + 0.35f) * radius * 0.8f,
                           body.y + std::sin(angle + 0.35f) * radius * 0.8f};
        const Vector2 right{body.x + std::cos(angle - 0.35f) * radius * 0.8f,
                            body.y + std::sin(angle - 0.35f) * radius * 0.8f};
        drawTriangleAnyWinding(tip, left, right, color);
    }
    DrawCircleV(body, radius, color);
    DrawCircleV(body, radius * 0.55f, palette::BG_DEEP);
    DrawCircleV(body, radius * (0.22f + 0.06f * pulse(motion, 3.0f)), color);
}

void Renderer2D::drawFloor(const core::Level& level) const {
    const TierStyle style = tierStyle(level.spec().tier);
    const int size = level.maze().size();
    const float extent = static_cast<float>(size) * m_cellSize;
    DrawRectangleRec(Rectangle{0.0f, 0.0f, extent, extent}, style.floor);
    for (int line = 1; line < size; ++line) {
        const float offset = static_cast<float>(line) * m_cellSize;
        DrawLineEx(Vector2{offset, 0.0f}, Vector2{offset, extent}, 1.0f, style.gridLine);
        DrawLineEx(Vector2{0.0f, offset}, Vector2{extent, offset}, 1.0f, style.gridLine);
    }
}

void Renderer2D::drawWallCache(const core::Level& level) const {
    if (!m_wallCache.isValid()) {
        return;
    }
    const float padding = m_cellSize * CACHE_PADDING_CELLS;
    // While phasing the walls turn translucent. The cache holds premultiplied colour, so the
    // tint scales colour and alpha together.
    const float opacity = level.isPhasing() ? PHASE_WALL_OPACITY : 1.0f;
    const auto channel = static_cast<unsigned char>(255.0f * opacity);
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    DrawTextureRec(m_wallCache.get().texture, flippedSource(m_wallCache),
                   Vector2{-padding, -padding}, Color{channel, channel, channel, channel});
    EndBlendMode();
}

void Renderer2D::drawSecretDoors(const core::Level& level, float time) const {
    const float glow = 0.55f + 0.45f * pulse(time, 0.8f);
    const float thickness = std::max(MIN_WALL_PIXELS, WALL_THICKNESS * m_cellSize);
    for (const core::SecretDoor& door : level.secretDoors()) {
        // The door is the wall segment shared by door.cell and its neighbour.
        const core::Cell other = core::neighbour(door.cell, door.side);
        const Vector2 a = cellCenter(
            core::Vec2f{static_cast<float>(door.cell.x), static_cast<float>(door.cell.y)});
        const Vector2 b =
            cellCenter(core::Vec2f{static_cast<float>(other.x), static_cast<float>(other.y)});
        const Vector2 middle{(a.x + b.x) / 2.0f, (a.y + b.y) / 2.0f};
        const bool vertical =
            door.side == core::Direction::East || door.side == core::Direction::West;
        const Vector2 half =
            vertical ? Vector2{0.0f, m_cellSize / 2.0f} : Vector2{m_cellSize / 2.0f, 0.0f};
        DrawLineEx(Vector2{middle.x - half.x, middle.y - half.y},
                   Vector2{middle.x + half.x, middle.y + half.y}, thickness,
                   fade(palette::ACCENT_GREEN_DIM, 0.85f));
        drawGlow(middle, m_cellSize * 0.45f, fade(palette::ACCENT_GREEN, 0.35f * glow));
        // The rune: a small diamond with a stroke through it.
        const float rune = m_cellSize * 0.16f;
        DrawPolyLinesEx(middle, 4, rune, 0.0f, std::max(1.5f, m_cellSize * 0.05f),
                        fade(palette::ACCENT_GREEN, glow));
        DrawLineEx(Vector2{middle.x, middle.y - rune * 1.5f},
                   Vector2{middle.x, middle.y + rune * 1.5f}, std::max(1.0f, m_cellSize * 0.035f),
                   fade(palette::TEXT, glow));
    }
}

void Renderer2D::drawExit(const core::Maze& maze, float time) const {
    const core::Cell exit = maze.exit();
    const Vector2 center =
        cellCenter(core::Vec2f{static_cast<float>(exit.x), static_cast<float>(exit.y)});
    const float radius = m_cellSize * EXIT_RADIUS;
    const float beat = pulse(time, 0.9f);
    const float spin = time * EXIT_SPIN_DEGREES_PER_SECOND;
    drawGlow(center, radius * (2.0f + 0.4f * beat),
             fade(palette::ACCENT_GREEN, 0.30f + 0.2f * beat));
    DrawCircleV(center, radius * 0.62f, fade(palette::ACCENT_GREEN_DIM, 0.75f));
    for (int arc = 0; arc < 2; ++arc) {
        const float start = spin + static_cast<float>(arc) * 180.0f;
        DrawRing(center, radius * 0.72f, radius * 0.86f, start, start + 110.0f, RING_SEGMENTS,
                 palette::ACCENT_GREEN);
        const float inner = -spin * 1.6f + static_cast<float>(arc) * 180.0f;
        DrawRing(center, radius * 0.42f, radius * 0.50f, inner, inner + 80.0f, RING_SEGMENTS,
                 fade(palette::TEXT, 0.85f));
    }
    DrawCircleV(center, radius * (0.16f + 0.06f * beat), palette::TEXT);
}

void Renderer2D::drawPlayer(const core::Level& level, float time) const {
    const Vector2 center = cellCenter(level.player().visualPosition());
    const float radius = m_cellSize * PLAYER_RADIUS;
    const float facing = directionAngle(level.player().facing());
    if (level.isPhasing()) {
        // Shimmer: a flickering purple aura and a ghostly offset copy.
        const float shimmer = pulse(time, 7.0f);
        drawGlow(center, radius * (2.6f + 0.6f * shimmer), fade(palette::WALL_GLOW, 0.45f));
        DrawRingLines(center, radius * (1.3f + 0.3f * shimmer), radius * (1.4f + 0.3f * shimmer),
                      0.0f, 360.0f, RING_SEGMENTS, fade(palette::WALL_GLOW, 0.8f));
    }
    drawSkin(m_skin, center, radius, time, facing, &m_trail);
}

void Renderer2D::drawPerks(const core::Level& level, float time) const {
    const bool fogged = level.spec().hasFog;
    for (const core::PerkItem& perk : level.perks()) {
        if (perk.collected || (fogged && !level.fog().isExplored(perk.cell))) {
            continue;
        }
        const float bob = std::sin(time * 2.5f + static_cast<float>(perk.id)) * m_cellSize * 0.05f;
        Vector2 center = cellCenter(
            core::Vec2f{static_cast<float>(perk.cell.x), static_cast<float>(perk.cell.y)});
        center.y += bob;
        drawPerkIcon(perk.type, center, m_cellSize * PERK_RADIUS,
                     time + static_cast<float>(perk.id));
    }
}

void Renderer2D::drawCompassTrail(const core::Level& level, float time) const {
    const std::vector<core::Cell>& path = level.compassPath();
    if (path.size() < 2) {
        return;
    }
    const float remaining = level.effects().remaining(core::EffectType::CompassTrail);
    const float alpha = std::min(1.0f, remaining / 0.5f); // fade out during the last half second
    for (std::size_t index = 1; index < path.size(); ++index) {
        const Vector2 from = cellCenter(core::Vec2f{static_cast<float>(path[index - 1].x),
                                                    static_cast<float>(path[index - 1].y)});
        const Vector2 to = cellCenter(
            core::Vec2f{static_cast<float>(path[index].x), static_cast<float>(path[index].y)});
        DrawLineEx(from, to, m_cellSize * 0.10f, fade(palette::ACCENT_GREEN, 0.35f * alpha));
        // A bright pulse runs along the trail towards the exit.
        const float wave = pulse(time * 1.5f - static_cast<float>(index) * 0.12f, 1.0f);
        DrawCircleV(to, m_cellSize * (0.07f + 0.05f * wave), fade(palette::ACCENT_GREEN, alpha));
    }
}

} // namespace render
