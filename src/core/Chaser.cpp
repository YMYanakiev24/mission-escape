#include "core/Chaser.h"

#include "core/Config.h"
#include "core/Pathfinding.h"

#include <algorithm>
#include <vector>

namespace core {

void Chaser::activate(Cell cell, float activeSeconds) {
    placeAt(cell);
    m_active = true;
    m_activeSeconds = std::max(0.0f, activeSeconds);
}

void Chaser::advance(float deltaSeconds, const Maze& maze, Cell target, float stepSeconds,
                     bool frozen) {
    if (!m_active || frozen) {
        return;
    }
    m_activeSeconds += deltaSeconds;
    update(deltaSeconds);
    if (isMoving()) {
        return;
    }
    // A fresh BFS for every step: the player may have moved since the last one.
    const std::optional<Direction> step = nextStepToward(maze, cell(), target, PassRules{});
    if (step) {
        beginMove(neighbour(cell(), *step), stepSeconds);
    }
}

Cell Chaser::chooseSpawnCell(const Maze& maze, Cell player, Rng& rng) {
    const Grid2D<int> fromPlayer = bfsDistances(maze, player, PassRules{});
    const Grid2D<int> fromExit = bfsDistances(maze, maze.exit(), PassRules{});
    const int minimumDistance = std::max(1, maze.size() / 2);
    std::vector<Cell> candidates;
    Cell farthest = player;
    for (int y = 0; y < maze.size(); ++y) {
        for (int x = 0; x < maze.size(); ++x) {
            const int distance = fromPlayer[y][x];
            if (distance > fromPlayer[farthest.y][farthest.x]) {
                farthest = Cell{x, y};
            }
            const bool farEnough = distance >= minimumDistance;
            const bool awayFromExit = fromExit[y][x] >= CHASER_EXIT_CLEARANCE;
            if (farEnough && awayFromExit) {
                candidates.push_back(Cell{x, y});
            }
        }
    }
    if (candidates.empty()) {
        return farthest;
    }
    return candidates[static_cast<std::size_t>(
        rng.nextInt(0, static_cast<int>(candidates.size()) - 1))];
}

float Chaser::speedFactor(float baseSpeed, float activeSeconds) {
    return std::min(CHASER_SPEED_MAX,
                    baseSpeed + CHASER_SPEED_RAMP_PER_SECOND * std::max(0.0f, activeSeconds));
}

} // namespace core
