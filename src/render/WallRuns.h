#pragma once

#include "core/Maze.h"

#include <raylib.h>

#include <vector>

/// @file WallRuns.h
/// Merges collinear wall segments into long straight runs. The 2D renderer draws one glowing
/// line per run and the 3D renderer builds one box per run, so neither has to deal with
/// thousands of tiny segments.

namespace render {

/// A straight wall run in cell-corner coordinates: (x, y) is the top-left corner of cell (x, y).
struct WallRun {
    Vector2 start;
    Vector2 end;
};

/// All plain walls of @p maze as merged runs. Secret doors are left out (they are drawn
/// separately with their rune).
std::vector<WallRun> computeWallRuns(const core::Maze& maze);

} // namespace render
