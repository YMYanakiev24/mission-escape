#pragma once

#include "core/MazeGenerator.h"
#include "core/Random.h"

#include <raylib.h>

#include <memory>

/// @file MenuBackground.h
/// Animated backdrop of the menus: a maze that generates itself with the real DFS algorithm.

namespace render {

/// Steps a core::DfsMazeBuilder a few cells per frame and draws the growing passage tree,
/// highlighting the current DFS stack. When the maze is complete it lingers, fades and restarts.
class MenuBackground {
public:
    /// Starts generating the first background maze.
    MenuBackground();

    /// Advances the animation.
    void update(float deltaSeconds);

    /// Draws the backdrop over the whole window (including the clear).
    void draw(float time) const;

private:
    /// Where the background maze is placed on screen.
    struct Layout {
        Vector2 origin;
        float cellSize;
        Vector2 centerOf(core::Cell cell) const;
    };

    void restart();
    void drawPassages(const Layout& layout, float opacity) const;
    void drawStackTrail(const Layout& layout, float fadeOut, float time) const;
    void drawVignette(float width, float height) const;

    core::Rng m_rng;
    std::unique_ptr<core::DfsMazeBuilder> m_builder;
    float m_stepBudget = 0.0f;
    float m_finishedTime = 0.0f;
};

} // namespace render
