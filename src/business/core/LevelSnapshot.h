#pragma once

#include "core/Cell.h"
#include "core/Perks.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

/// @file LevelSnapshot.h
/// Everything needed to resume a level exactly where the player left it.

namespace core {

/// In-progress state of one level. The maze itself is not stored: it is regenerated from
/// the seed, which is why generation must be deterministic.
struct LevelSnapshot {
    int level = 1;
    std::uint32_t seed = 0;
    Cell playerCell;
    Direction facing = Direction::East;
    int elapsedMs = 0;
    bool timerStarted = false;
    int berries = 0;
    std::array<int, EFFECT_TYPE_COUNT> effectMs{}; ///< Remaining time of each effect.
    std::vector<int> collectedPerkIds;
    std::string exploredHex; ///< Fog memory, 4 cells per hex digit, row-major.
    bool chaserActive = false;
    Cell chaserCell;
    int chaserActiveMs = 0; ///< How long the chaser has been hunting (speed ramp).
    int graceMs = 0;        ///< Remaining grace time before the chaser appears.
    int steps = 0;          ///< Moves made so far in this attempt.
};

} // namespace core
