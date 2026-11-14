#pragma once

#include <cstdint>
#include <string_view>

/// @file LevelRules.h
/// The campaign layout: 20 levels in 4 tiers, and the rules each tier applies.

namespace core {

/// Difficulty tier; each tier spans five consecutive levels.
enum class Tier : std::uint8_t { Awakening, Labyrinth, Fog, Hunted };

/// How the player looks at the maze. Logic is identical, only rendering and input differ.
enum class ViewMode : std::uint8_t { Classic2D, FirstPerson3D };

/// Everything the generator and the level simulation need to know about one level.
struct LevelSpec {
    int level = 1;
    Tier tier = Tier::Awakening;
    int candidateCount = 1;  ///< Mazes generated; the hardest one is kept.
    bool exitDecoys = false; ///< Hang dead-end branches next to the exit.
    float braidRatio = 0.0f; ///< Fraction of dead ends opened into loops.
    int secretDoorMin = 0;   ///< Inclusive range of secret doors to place.
    int secretDoorMax = 0;
    bool hasFog = false;          ///< Fog of war (tier 3).
    bool hasChaser = false;       ///< Hunting chaser (tier 4).
    float chaserBaseSpeed = 0.0f; ///< Chaser speed as a fraction of the player's speed.
};

/// True for level numbers 1..LEVEL_COUNT.
bool isValidLevel(int level);

/// Tier that contains @p level (levels outside the range are clamped).
Tier tierForLevel(int level);

/// 1-based tier number (1..4) used in the UI and save files.
int tierNumber(Tier tier);

/// First level of a tier (1, 6, 11, 16).
int firstLevelOfTier(Tier tier);

/// Display name of a tier ("Awakening", "Labyrinth", "Fog", "Hunted").
std::string_view tierName(Tier tier);

/// One-sentence description shown on the tier intro card.
std::string_view tierDescription(Tier tier);

/// Full rule set for @p level.
LevelSpec levelSpec(int level);

/// Name used in save files ("2d" / "3d").
std::string_view viewModeKey(ViewMode mode);

/// Display name ("2D Classic" / "3D First-Person").
std::string_view viewModeName(ViewMode mode);

/// Seconds needed to move one cell in @p mode.
float moveSecondsFor(ViewMode mode);

} // namespace core
