#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string_view>

/// @file Cell.h
/// Grid coordinates, compass directions and small helpers shared by the whole core.

namespace core {

/// One of the four compass directions. North is "up" on screen (decreasing y).
enum class Direction : std::uint8_t { North, East, South, West };

inline constexpr int DIRECTION_COUNT = 4;

/// All directions in a fixed order (N, E, S, W); iteration order matters for determinism.
inline constexpr std::array<Direction, DIRECTION_COUNT> ALL_DIRECTIONS = {
    Direction::North, Direction::East, Direction::South, Direction::West};

/// Integer coordinates of a maze cell; (0,0) is the top-left cell.
struct Cell {
    int x = 0;
    int y = 0;
};

/// Continuous position in cell units, used for interpolated (tweened) movement.
struct Vec2f {
    float x = 0.0f;
    float y = 0.0f;
};

constexpr bool operator==(Cell lhs, Cell rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

constexpr bool operator!=(Cell lhs, Cell rhs) {
    return !(lhs == rhs);
}

/// Horizontal offset of one step in @p direction.
constexpr int deltaX(Direction direction) {
    switch (direction) {
        case Direction::East:
            return 1;
        case Direction::West:
            return -1;
        default:
            return 0;
    }
}

/// Vertical offset of one step in @p direction.
constexpr int deltaY(Direction direction) {
    switch (direction) {
        case Direction::South:
            return 1;
        case Direction::North:
            return -1;
        default:
            return 0;
    }
}

/// The cell reached by taking one step from @p cell towards @p direction (may be out of bounds).
constexpr Cell neighbour(Cell cell, Direction direction) {
    return Cell{cell.x + deltaX(direction), cell.y + deltaY(direction)};
}

/// The direction pointing the other way.
constexpr Direction opposite(Direction direction) {
    return static_cast<Direction>((static_cast<int>(direction) + 2) % DIRECTION_COUNT);
}

/// The direction after a 90 degree counter-clockwise turn.
constexpr Direction turnedLeft(Direction direction) {
    return static_cast<Direction>((static_cast<int>(direction) + 3) % DIRECTION_COUNT);
}

/// The direction after a 90 degree clockwise turn.
constexpr Direction turnedRight(Direction direction) {
    return static_cast<Direction>((static_cast<int>(direction) + 1) % DIRECTION_COUNT);
}

/// Manhattan (taxicab) distance between two cells.
inline int manhattanDistance(Cell a, Cell b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

/// Lower-case name used in save files ("north", "east", ...).
std::string_view directionName(Direction direction);

/// Parses a name produced by directionName(); returns nothing for unknown text.
std::optional<Direction> parseDirection(std::string_view text);

} // namespace core
