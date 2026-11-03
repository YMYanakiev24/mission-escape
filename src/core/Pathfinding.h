#pragma once

#include "core/Cell.h"
#include "core/Grid2D.h"
#include "core/Maze.h"

#include <optional>
#include <vector>

/// @file Pathfinding.h
/// Breadth-first search utilities: distance maps, shortest paths and single chase steps.

namespace core {

/// Distance value of cells that cannot be reached.
inline constexpr int UNREACHABLE = -1;

/// BFS distance (in steps) from @p source to every cell, or ::UNREACHABLE.
Grid2D<int> bfsDistances(const Maze& maze, Cell source, const PassRules& rules);

/// Number of steps of the shortest path between two cells, or ::UNREACHABLE.
int shortestPathLength(const Maze& maze, Cell from, Cell to, const PassRules& rules);

/// Shortest path from @p from to @p to including both end cells; empty if unreachable.
/// Ties are broken by the fixed N/E/S/W order, so the result is deterministic.
std::vector<Cell> shortestPath(const Maze& maze, Cell from, Cell to, const PassRules& rules);

/// Shortest path that reuses a distance map computed from @p to (see bfsDistances()).
std::vector<Cell> pathFromDistanceMap(const Maze& maze, const Grid2D<int>& distancesToTarget,
                                      Cell from, const PassRules& rules);

/// The first step of a shortest path from @p from towards @p target; nothing if already there
/// or if @p target is unreachable. Every returned step decreases the distance by exactly one.
std::optional<Direction> nextStepToward(const Maze& maze, Cell from, Cell target,
                                        const PassRules& rules);

/// True when every cell can be reached from the start cell.
bool allCellsReachable(const Maze& maze, const PassRules& rules);

} // namespace core
