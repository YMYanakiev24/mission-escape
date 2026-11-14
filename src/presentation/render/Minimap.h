#pragma once

#include "core/Level.h"
#include "core/Progress.h"
#include "render/GpuResources.h"
#include "render/Theme.h"

#include <raylib.h>

/// @file Minimap.h
/// The 3D mode's map in the top-right corner.

namespace render {

/// Shows the explored part of the maze, the player arrow and what has been seen so far.
///
/// The explored walls are drawn into a cached texture that is only redrawn when the number of
/// explored cells changes; markers (player, exit, perks, chaser) are drawn live on top.
class Minimap {
public:
    /// Switches between the small and the large map (M key).
    void toggleSize() noexcept { m_large = !m_large; }

    /// Forgets the cached map (new level).
    void reset();

    /// Draws the map below the HUD at the top-right corner.
    void draw(const Theme& theme, const core::Level& level, float yaw, core::SkinId skin,
              float time);

private:
    Rectangle bounds(const Theme& theme) const;
    void rebuildCache(const core::Level& level, int pixels);
    Vector2 cellCenter(Rectangle area, int mazeSize, core::Vec2f cell) const;
    void drawMarkers(Rectangle area, const core::Level& level, float time) const;
    void drawPlayerArrow(Rectangle area, const core::Level& level, float yaw, Color color) const;

    bool m_large = false;
    RenderTarget m_cache;
    int m_cachedExplored = -1;
    int m_cachedPixels = 0;
};

} // namespace render
