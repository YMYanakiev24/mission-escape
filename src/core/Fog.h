#pragma once

#include "core/Cell.h"
#include "core/Grid2D.h"
#include "core/Maze.h"

#include <string>

/// @file Fog.h
/// Fog of war: what the player sees right now and what they remember.

namespace core {

/// Visibility and exploration memory of the player, one flag per cell.
///
/// Visibility is a flood fill through open passages up to a radius, so the player can never
/// see through walls (secret doors count as walls). Explored cells stay remembered.
class Fog {
public:
    /// Creates an empty fog for no maze.
    Fog() = default;

    /// Creates fog for an N x N maze with nothing seen yet.
    explicit Fog(int size);

    /// Replaces the currently visible set by the cells within @p radius passage steps of
    /// @p center and adds them to the explored memory.
    void reveal(const Maze& maze, Cell center, int radius);

    /// Additionally marks cells visible along straight open corridors from @p center
    /// (used for the 3D minimap in tiers without fog).
    void revealCorridors(const Maze& maze, Cell center, int maxDistance);

    /// True when the cell is currently in sight.
    bool isVisible(Cell cell) const;

    /// True when the cell has been seen at least once.
    bool isExplored(Cell cell) const;

    /// Number of explored cells.
    int exploredCount() const;

    /// Explored flags (row-major Grid2D).
    const Grid2D<bool>& explored() const noexcept { return m_explored; }

    /// Encodes the explored map as hexadecimal text (4 cells per digit, row-major).
    std::string encodeExplored() const;

    /// Restores the explored map from encodeExplored() text; returns false (and changes
    /// nothing) if the text has the wrong length or invalid characters.
    bool decodeExplored(const std::string& hex);

private:
    void markSeen(Cell cell);

    Grid2D<bool> m_explored;
    Grid2D<bool> m_visible;
};

} // namespace core
