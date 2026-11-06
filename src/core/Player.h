#pragma once

#include "core/Cell.h"

/// @file Player.h
/// Grid-locked mover with smooth (tweened) motion between cells.

namespace core {

/// A grid-locked mover that travels from cell to cell over a fixed duration.
///
/// The logical cell changes as soon as a move starts (so rules such as collisions use the
/// destination), while visualPosition() interpolates between the previous and current cell.
/// The player and the chaser both use this class.
class Mover {
public:
    /// Places the mover at rest on @p cell.
    explicit Mover(Cell cell = Cell{});

    /// Logical cell (the destination while a move is in progress).
    Cell cell() const noexcept { return m_cell; }

    /// Cell the current move started from (equal to cell() when at rest).
    Cell previousCell() const noexcept { return m_previousCell; }

    /// True while a move is being animated.
    bool isMoving() const noexcept { return m_progress < 1.0f; }

    /// Linear progress of the current move in [0, 1] (1 when at rest).
    float moveProgress() const noexcept { return m_progress; }

    /// Interpolated position in cell units (cell centres are at integer coordinates).
    Vec2f visualPosition() const;

    /// Starts moving to the adjacent cell @p target over @p durationSeconds.
    void beginMove(Cell target, float durationSeconds);

    /// Advances the current move; returns true on the frame the move completes.
    bool update(float deltaSeconds);

    /// Puts the mover at rest on @p cell immediately (used for spawning and snapshots).
    void placeAt(Cell cell);

private:
    Cell m_cell;
    Cell m_previousCell;
    float m_progress = 1.0f;
    float m_duration = 1.0f;
};

/// The player: a Mover with a facing direction and a Magic Berry inventory.
class Player : public Mover {
public:
    /// Places the player on @p start facing @p facing with an empty inventory.
    explicit Player(Cell start = Cell{}, Direction facing = Direction::East);

    /// Direction the player looks at (used by 3D mode and skin orientation).
    Direction facing() const noexcept { return m_facing; }

    /// Changes the facing direction.
    void setFacing(Direction facing) noexcept { m_facing = facing; }

    /// Magic Berries carried (0..BERRY_CAPACITY).
    int berries() const noexcept { return m_berries; }

    /// Adds one berry; returns false (and changes nothing) when the inventory is full.
    bool addBerry();

    /// Removes one berry; returns false when there is none.
    bool consumeBerry();

    /// Sets the berry count, clamped to the valid range (used when restoring snapshots).
    void setBerries(int count);

private:
    Direction m_facing;
    int m_berries = 0;
};

} // namespace core
