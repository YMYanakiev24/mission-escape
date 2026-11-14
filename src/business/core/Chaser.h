#pragma once

#include "core/Cell.h"
#include "core/Maze.h"
#include "core/Player.h"
#include "core/Random.h"

/// @file Chaser.h
/// The tier 4 hunter: walks the shortest path towards the player, one cell at a time.

namespace core {

/// A Mover that, whenever it stands still, takes one BFS step towards its target. The path is
/// recomputed for every step, so it reacts to the player immediately. It never uses secret
/// doors and can never phase through walls.
class Chaser : public Mover {
public:
    /// True once the chaser has appeared.
    bool isActive() const noexcept { return m_active; }

    /// Seconds the chaser has been hunting (drives the speed ramp).
    float activeSeconds() const noexcept { return m_activeSeconds; }

    /// Places the chaser on @p cell and starts the hunt.
    void activate(Cell cell, float activeSeconds = 0.0f);

    /// Advances the hunt. While @p frozen the chaser does not move at all.
    void advance(float deltaSeconds, const Maze& maze, Cell target, float stepSeconds, bool frozen);

    /// Picks a spawn cell at least N/2 path steps from @p player and away from the exit;
    /// falls back to the farthest reachable cell.
    static Cell chooseSpawnCell(const Maze& maze, Cell player, Rng& rng);

    /// Speed relative to the player: the level's base speed plus a slow ramp, capped.
    static float speedFactor(float baseSpeed, float activeSeconds);

private:
    bool m_active = false;
    float m_activeSeconds = 0.0f;
};

} // namespace core
