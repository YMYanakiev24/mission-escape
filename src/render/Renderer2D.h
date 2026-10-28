#pragma once

#include "core/Cell.h"
#include "core/Grid2D.h"
#include "core/Level.h"
#include "core/Progress.h"
#include "render/GpuResources.h"
#include "render/SkinRenderer.h"
#include "render/WallRuns.h"

#include <raylib.h>

#include <optional>
#include <vector>

/// @file Renderer2D.h
/// Top-down neon rendering of a level. Reads core state, never changes game rules.

namespace render {

/// Draws the maze, the exit portal, the player and overlays for one level in 2D.
///
/// The static walls are rendered once per level (and per cell size) into an off-screen
/// texture with a layered neon glow; each frame only blits that texture and draws the
/// animated elements on top, so even a 50 x 50 maze costs a handful of draw calls.
class Renderer2D {
public:
    /// Smallest cell size (pixels) at which the whole maze is shown at once.
    static constexpr float MIN_FIT_CELL_SIZE = 24.0f;

    /// Drops cached textures so the next update() rebuilds them for a new level.
    void beginLevel();

    /// Computes the layout for @p viewport, follows the player and refreshes caches.
    void update(float deltaSeconds, const core::Level& level, Rectangle viewport, float uiScale);

    /// Draws the level inside the viewport given to update().
    void draw(const core::Level& level, float time) const;

    /// The maze cell under a screen position, if any.
    std::optional<core::Cell> cellAt(Vector2 screenPosition) const;

    /// Screen position of a point given in cell units (cell centres at x + 0.5).
    Vector2 toScreen(core::Vec2f cellPosition) const;

    /// Current size of one cell in pixels.
    float cellSize() const noexcept { return m_cellSize; }

    /// True when the camera follows the player instead of showing the whole maze.
    bool followsPlayer() const noexcept { return m_follow; }

    /// Offsets the camera for this frame (screen shake when caught).
    void setShake(Vector2 offset) noexcept { m_shake = offset; }

    /// Chooses the skin used to draw the player.
    void setSkin(core::SkinId skin) noexcept { m_skin = skin; }

private:
    void computeLayout(int mazeSize, Rectangle viewport, float uiScale);
    void updateCamera(float deltaSeconds, const core::Level& level, bool snap);
    void updateFog(float deltaSeconds, const core::Level& level, bool snap);
    void rebuildWallCache(const core::Maze& maze);
    void drawRuns(float thickness, Color color) const;
    void drawFloor(const core::Level& level) const;
    void drawWallCache(const core::Level& level) const;
    void drawSecretDoors(const core::Level& level, float time) const;
    void drawExit(const core::Maze& maze, float time) const;
    void drawPlayer(const core::Level& level, float time) const;
    void drawChaser(const core::Level& level, float time) const;
    void drawPerks(const core::Level& level, float time) const;
    void drawCompassTrail(const core::Level& level, float time) const;
    void drawFog(const core::Level& level) const;
    void drawExitHint(const core::Level& level, float time) const;
    void drawPlayerLight(const core::Level& level, float time) const;
    Vector2 cellCenter(core::Vec2f cellPosition) const;

    Rectangle m_viewport{};
    Vector2 m_shake{};
    core::SkinId m_skin = core::SkinId::Wisp;
    SkinTrail m_trail;              ///< Comet tail positions in world pixels.
    core::Grid2D<float> m_fogAlpha; /// Smoothed darkness of every cell (0 = clear, 1 = unknown).
    int m_mazeSize = 0;
    float m_cellSize = MIN_FIT_CELL_SIZE;
    bool m_follow = false;
    bool m_needsRebuild = true;
    bool m_cameraInitialised = false;
    float m_cachedCellSize = 0.0f;
    Camera2D m_camera{};
    std::vector<WallRun> m_runs;
    RenderTarget m_wallCache;
    RenderTarget m_scratch;
};

} // namespace render
