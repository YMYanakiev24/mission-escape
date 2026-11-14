#pragma once

#include "core/Cell.h"
#include "core/Grid2D.h"

#include <cstdint>

/// @file Maze.h
/// Storage and maintenance of the maze layout (walls and secret doors), without any drawing.

namespace core {

/// Bit layout of one cell: bits 0-3 are walls N/E/S/W, bits 4-7 mark secret doors N/E/S/W.
/// A secret door always keeps its wall bit set, so code that ignores secret doors sees a wall.
namespace cellbits {
inline constexpr std::uint8_t WALL_NORTH = 1U << 0U;
inline constexpr std::uint8_t WALL_EAST = 1U << 1U;
inline constexpr std::uint8_t WALL_SOUTH = 1U << 2U;
inline constexpr std::uint8_t WALL_WEST = 1U << 3U;
inline constexpr std::uint8_t ALL_WALLS = WALL_NORTH | WALL_EAST | WALL_SOUTH | WALL_WEST;
inline constexpr int SECRET_SHIFT = 4;
} // namespace cellbits

/// Wall bit of @p direction inside a cell bitmask.
constexpr std::uint8_t wallBit(Direction direction) {
    return static_cast<std::uint8_t>(1U << static_cast<unsigned>(direction));
}

/// Secret-door bit of @p direction inside a cell bitmask.
constexpr std::uint8_t secretBit(Direction direction) {
    return static_cast<std::uint8_t>(wallBit(direction) << cellbits::SECRET_SHIFT);
}

/// Which wall exceptions a mover may use. The outer boundary is never passable.
struct PassRules {
    bool useSecretDoors = false;    ///< Walk through secret doors (the player can, the chaser not).
    bool phaseThroughWalls = false; ///< Magic Berry phase: every inner wall is passable.
};

/// Square N x N maze stored as one wall bitmask per cell in a Grid2D.
///
/// Start is always the top-left cell (0,0) and the exit the bottom-right cell (N-1,N-1).
/// Walls are kept consistent on both sides: removing the east wall of a cell also removes the
/// west wall of its neighbour.
class Maze {
public:
    /// Creates an empty maze with no cells.
    Maze() = default;

    /// Creates an N x N maze in which every cell is fully walled in.
    /// @throws std::invalid_argument if @p size is negative.
    explicit Maze(int size);

    /// Side length N of the maze.
    int size() const noexcept { return m_cells.width(); }

    /// True when @p cell lies inside the maze.
    bool inBounds(Cell cell) const noexcept { return m_cells.inBounds(cell.x, cell.y); }

    /// The start cell, always (0,0).
    Cell start() const noexcept { return Cell{0, 0}; }

    /// The exit cell, always (N-1, N-1).
    Cell exit() const noexcept { return Cell{size() - 1, size() - 1}; }

    /// Raw bitmask of a cell (see ::core::cellbits).
    std::uint8_t cellBits(Cell cell) const { return m_cells.at(cell.x, cell.y); }

    /// True when a wall (plain or secret door) blocks @p direction of @p cell.
    bool hasWall(Cell cell, Direction direction) const;

    /// True when the wall on @p direction of @p cell is a secret door.
    bool hasSecretDoor(Cell cell, Direction direction) const;

    /// True when @p direction of @p cell faces the outside of the maze.
    bool isBoundary(Cell cell, Direction direction) const;

    /// True when an open passage (no wall at all) leads from @p cell towards @p direction.
    bool isOpen(Cell cell, Direction direction) const;

    /// True when a mover obeying @p rules may step from @p cell towards @p direction.
    bool canPass(Cell cell, Direction direction, const PassRules& rules) const;

    /// Removes the wall between @p cell and its neighbour (both sides, secret flags too).
    /// Boundary walls are never removed.
    void carvePassage(Cell cell, Direction direction);

    /// Restores a plain wall between @p cell and its neighbour (clears secret flags).
    void addWall(Cell cell, Direction direction);

    /// Turns the inner wall between @p cell and its neighbour into a secret door.
    /// @return false if there is no inner wall at that position.
    bool setSecretDoor(Cell cell, Direction direction);

    /// Number of open passages around @p cell (secret doors do not count).
    int openingCount(Cell cell) const;

    /// True when exactly one open passage leads out of @p cell.
    bool isDeadEnd(Cell cell) const { return openingCount(cell) == 1; }

    /// Total number of open passages, each shared wall counted once.
    int passageCount() const;

    /// Number of dead-end cells.
    int deadEndCount() const;

    /// Read-only access to the underlying bitmask grid.
    const Grid2D<std::uint8_t>& cells() const noexcept { return m_cells; }

    friend bool operator==(const Maze& lhs, const Maze& rhs) { return lhs.m_cells == rhs.m_cells; }
    friend bool operator!=(const Maze& lhs, const Maze& rhs) { return !(lhs == rhs); }

private:
    void setBits(Cell cell, std::uint8_t bits) { m_cells.at(cell.x, cell.y) |= bits; }
    void clearBits(Cell cell, std::uint8_t bits);

    Grid2D<std::uint8_t> m_cells;
};

} // namespace core
