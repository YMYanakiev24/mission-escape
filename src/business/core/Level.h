#pragma once

#include "core/Cell.h"
#include "core/Chaser.h"
#include "core/Fog.h"
#include "core/LevelRules.h"
#include "core/LevelSnapshot.h"
#include "core/Maze.h"
#include "core/MazeGenerator.h"
#include "core/Perks.h"
#include "core/Player.h"
#include "core/Random.h"

#include <cstdint>
#include <optional>
#include <vector>

/// @file Level.h
/// The playable simulation of one level: pure game rules, no rendering and no I/O.

namespace core {

/// Overall state of a level attempt.
enum class LevelState : std::uint8_t { Playing, Won, Caught };

/// Result of a movement command.
enum class MoveResult : std::uint8_t {
    Moved,   ///< The move started.
    Blocked, ///< A wall is in the way.
    Busy,    ///< The previous move is still animating.
    Inactive ///< The level is over; input is ignored.
};

/// Something that happened during a frame; the app turns events into sound, particles, toasts.
enum class LevelEventType : std::uint8_t {
    TimerStarted,
    Moved,
    SecretDoorUsed,
    PerkCollected, ///< See LevelEvent::perk.
    InventoryFull, ///< A Magic Berry was left on the floor because 3 are carried.
    PhaseStarted,
    PhaseEnded,
    NoBerries, ///< Shift was pressed with an empty inventory.
    ChaserSpawned,
    Caught,
    Won,
};

/// One gameplay event with the cell where it happened.
struct LevelEvent {
    LevelEventType type = LevelEventType::Moved;
    Cell cell;
    PerkType perk = PerkType::MagicBerry; ///< Only meaningful for PerkCollected.
};

/// One attempt at a level: owns the generated maze, the player, the fog, the chaser and the
/// timer.
///
/// Input is translated by the app into commands (tryMove(), bufferMove(), turnLeft(), ...);
/// update() advances time. Everything here is deterministic for a given seed and command
/// sequence, which is what the unit tests rely on.
class Level {
public:
    /// Generates level @p levelNumber for an N x N maze from @p seed.
    Level(int levelNumber, int mazeSize, std::uint32_t seed, ViewMode mode);

    /// Level number 1..20.
    int levelNumber() const noexcept { return m_spec.level; }
    /// Tier rules of this level.
    const LevelSpec& spec() const noexcept { return m_spec; }
    /// 2D or 3D (changes movement durations and the par time).
    ViewMode viewMode() const noexcept { return m_mode; }
    /// Seed the maze was generated from.
    std::uint32_t seed() const noexcept { return m_seed; }
    /// The generated maze.
    const Maze& maze() const noexcept { return m_maze; }
    /// The player (position, facing, berries).
    const Player& player() const noexcept { return m_player; }
    /// Visibility and exploration memory.
    const Fog& fog() const noexcept { return m_fog; }
    /// The tier 4 chaser (inactive in other tiers).
    const Chaser& chaser() const noexcept { return m_chaser; }
    /// Secret doors placed in this maze.
    const std::vector<SecretDoor>& secretDoors() const noexcept { return m_secretDoors; }
    /// Perks of this level (collected ones are flagged).
    const std::vector<PerkItem>& perks() const noexcept { return m_perks; }
    /// Remaining time of every timed effect.
    const EffectTimers& effects() const noexcept { return m_effects; }

    /// Shortest path from the player to the exit while a Compass Spark is active (else empty).
    const std::vector<Cell>& compassPath() const noexcept { return m_compassPath; }

    /// True while a Magic Berry phase lets the player pass through inner walls.
    bool isPhasing() const { return m_effects.isActive(EffectType::Phase); }
    /// Playing, won or caught.
    LevelState state() const noexcept { return m_state; }

    /// True once the level is won or lost.
    bool isFinished() const noexcept { return m_state != LevelState::Playing; }

    /// Seconds on the level timer (starts with the first move).
    float elapsedSeconds() const noexcept { return m_elapsed; }

    /// True after the first successful move.
    bool timerStarted() const noexcept { return m_timerStarted; }

    /// Door-free shortest path from start to exit in steps (basis of the par time).
    int shortestPathLength() const noexcept { return m_shortestPathLength; }

    /// Moves made so far in this attempt; bumps into walls and ignored input do not count.
    int stepsTaken() const noexcept { return m_stepsTaken; }

    /// Par time in seconds for this level, tier and view mode.
    float parTimeSeconds() const noexcept { return m_parTime; }

    /// Stars earned; only meaningful once the level is won.
    int earnedStars() const noexcept { return m_earnedStars; }

    /// Current sight radius in cells (fog levels).
    int fogRadius() const;

    /// True once the exit cell has been seen (fog levels show a faint hint until then).
    bool isExitSeen() const { return m_fog.isExplored(m_maze.exit()); }

    /// Seconds until the chaser appears (only meaningful in chaser levels).
    float chaserGraceRemaining() const noexcept { return m_graceRemaining; }

    /// 0 when the chaser is absent or far away, rising to 1 when it is adjacent.
    float chaserDanger() const;

    /// Path distance between chaser and player, or UNREACHABLE (-1) when there is none.
    int chaserDistance() const noexcept { return m_chaserDistance; }

    /// Tries to start a move; walls block unless a rule allows passing.
    MoveResult tryMove(Direction direction);

    /// Remembers one move to perform as soon as the current move completes.
    void bufferMove(Direction direction);

    /// Eats a Magic Berry: phase through inner walls for PHASE_SECONDS. Returns false when no
    /// berry is carried or a phase is already running.
    bool activateBerry();

    /// Rotates the facing direction 90 degrees counter-clockwise (3D controls).
    void turnLeft();

    /// Rotates the facing direction 90 degrees clockwise (3D controls).
    void turnRight();

    /// Advances animations, timers, the fog and the chaser by @p deltaSeconds.
    void update(float deltaSeconds);

    /// Returns and clears the events produced since the last call.
    std::vector<LevelEvent> takeEvents();

    /// Captures everything needed to resume this attempt later.
    LevelSnapshot snapshot() const;

    /// Restores a snapshot taken from the same level number and seed. Every field is checked
    /// against this maze first; on any inconsistency nothing changes and false is returned.
    bool restore(const LevelSnapshot& saved);

private:
    PassRules playerRules() const;
    void startMove(Direction direction);
    void onPlayerArrived();
    void advanceTimer(float deltaSeconds);
    void updateVisibility();
    void updateChaser(float deltaSeconds);
    void checkCaught();
    void collectPerkAt(Cell cell);
    void applyPerk(PerkType type);
    void updateEffects(float deltaSeconds);
    void refreshCompassPath();
    bool restorePerksAndEffects(const LevelSnapshot& saved);
    void win();
    void emit(LevelEventType type, Cell cell, PerkType perk = PerkType::MagicBerry);

    LevelSpec m_spec;
    ViewMode m_mode;
    std::uint32_t m_seed;
    Maze m_maze;
    std::vector<SecretDoor> m_secretDoors;
    std::vector<PerkItem> m_perks;
    EffectTimers m_effects;
    std::vector<Cell> m_compassPath;
    Player m_player;
    Fog m_fog;
    Chaser m_chaser;
    Rng m_spawnRng;
    LevelState m_state = LevelState::Playing;
    float m_elapsed = 0.0f;
    bool m_timerStarted = false;
    std::optional<Direction> m_bufferedMove;
    int m_shortestPathLength = 0;
    int m_stepsTaken = 0;
    float m_parTime = 0.0f;
    int m_earnedStars = 0;
    float m_graceRemaining = 0.0f;
    int m_chaserDistance = -1;
    Cell m_lastRevealCell{-1, -1};
    int m_lastRevealRadius = -1;
    std::vector<LevelEvent> m_events;
};

} // namespace core
