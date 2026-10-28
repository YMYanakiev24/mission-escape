#pragma once

#include "core/Cell.h"
#include "core/Maze.h"

#include <ostream>

/// @file TestUtil.h
/// Small helpers shared by several test files.

namespace core {

/// Lets CHECK_EQ print cells as "(x,y)".
inline std::ostream& operator<<(std::ostream& stream, Cell cell) {
    return stream << '(' << cell.x << ',' << cell.y << ')';
}

} // namespace core

namespace testutil {

/// A maze with every inner wall removed (an open field).
inline core::Maze openMaze(int size) {
    core::Maze maze(size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            maze.carvePassage(core::Cell{x, y}, core::Direction::East);
            maze.carvePassage(core::Cell{x, y}, core::Direction::South);
        }
    }
    return maze;
}

/// A serpentine maze: rows are open corridors joined alternately at the right and left end.
/// For odd sizes its only start-to-exit path visits every cell (size*size - 1 steps).
inline core::Maze serpentineMaze(int size) {
    core::Maze maze(size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x + 1 < size; ++x) {
            maze.carvePassage(core::Cell{x, y}, core::Direction::East);
        }
        if (y + 1 < size) {
            const int joinX = (y % 2 == 0) ? size - 1 : 0;
            maze.carvePassage(core::Cell{joinX, y}, core::Direction::South);
        }
    }
    return maze;
}

/// True when two cells are grid neighbours.
inline bool areAdjacent(core::Cell a, core::Cell b) {
    return core::manhattanDistance(a, b) == 1;
}

} // namespace testutil
