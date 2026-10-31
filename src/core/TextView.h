#pragma once

#include "core/Cell.h"
#include "core/Grid2D.h"
#include "core/Maze.h"

#include <string>
#include <vector>

/// @file TextView.h
/// Converts a maze into an ASCII picture (the "Text View", toggled with T in game).

namespace core {

inline constexpr char TEXT_WALL = '#';
inline constexpr char TEXT_OPEN = ' ';
inline constexpr char TEXT_SECRET_DOOR = '%';
inline constexpr char TEXT_START = 'S';
inline constexpr char TEXT_EXIT = 'E';
inline constexpr char TEXT_PLAYER = '@';
inline constexpr char TEXT_CHASER = 'X';
inline constexpr char TEXT_PERK = '*';
inline constexpr char TEXT_UNKNOWN = '.';

/// A symbol drawn on top of a cell (player, chaser, perk...).
struct TextMarker {
    Cell cell;
    char symbol = TEXT_PLAYER;
};

/// What to overlay on the plain maze layout.
struct TextViewOptions {
    /// Drawn in order, so later markers win when several share a cell.
    std::vector<TextMarker> markers;
    /// Fog memory; cells that are not explored are drawn as ::TEXT_UNKNOWN. Null = all visible.
    const Grid2D<bool>* explored = nullptr;
};

/// Number of text columns (and rows) produced for a maze of side @p mazeSize: 2N + 1.
constexpr int textViewDimension(int mazeSize) {
    return 2 * mazeSize + 1;
}

/// Renders @p maze as (2N+1) lines of (2N+1) characters separated by '\n'.
///
/// Cell (x, y) maps to character (2x+1, 2y+1); the characters in between are walls ('#'),
/// openings (' ') or secret doors ('%'); even/even positions are wall corners.
/// This is a pure function: it performs no I/O, the caller decides how to display the text.
std::string mazeToText(const Maze& maze, const TextViewOptions& options = {});

} // namespace core
