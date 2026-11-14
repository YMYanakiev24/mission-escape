#pragma once

#include "core/Progress.h"

#include <raylib.h>

#include <vector>

/// @file SkinRenderer.h
/// Procedural, animated player skins.

namespace render {

/// Signature colour of a skin (used for glows, the 3D minimap marker and hand light).
Color skinColor(core::SkinId skin);

/// Recent positions of the player, newest last; the Comet skin draws its tail from it.
class SkinTrail {
public:
    /// Records @p position when it moved far enough from the last point; otherwise the tail
    /// slowly shrinks while standing still.
    void record(Vector2 position, float minimumSpacing);

    /// Forgets all points (new level).
    void clear() { m_points.clear(); }

    const std::vector<Vector2>& points() const noexcept { return m_points; }

private:
    std::vector<Vector2> m_points;
    int m_idleFrames = 0;
};

/// Draws @p skin centred on @p center.
/// @param facingRadians direction of travel (0 = east, increasing clockwise on screen).
/// @param trail         optional recent positions for the Comet tail; when null a short tail
///                      is synthesised from the facing direction (menu previews).
void drawSkin(core::SkinId skin, Vector2 center, float radius, float time, float facingRadians,
              const SkinTrail* trail = nullptr);

} // namespace render
