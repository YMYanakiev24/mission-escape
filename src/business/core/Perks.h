#pragma once

#include "core/Cell.h"
#include "core/LevelRules.h"
#include "core/Maze.h"
#include "core/Random.h"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

/// @file Perks.h
/// Perk types, their placement in the maze and the timed effects they trigger.

namespace core {

/// Collectible perk types.
enum class PerkType : std::uint8_t { MagicBerry, Hourglass, CompassSpark, Lantern, FrostShard };

inline constexpr int PERK_TYPE_COUNT = 5;

inline constexpr std::array<PerkType, PERK_TYPE_COUNT> ALL_PERK_TYPES = {
    PerkType::MagicBerry, PerkType::Hourglass, PerkType::CompassSpark, PerkType::Lantern,
    PerkType::FrostShard};

/// A perk placed on the maze.
struct PerkItem {
    int id = 0; ///< Stable index used by save snapshots.
    PerkType type = PerkType::MagicBerry;
    Cell cell;
    bool collected = false;
};

/// Display name ("Magic Berry", ...).
std::string_view perkName(PerkType type);

/// One-line description used in tooltips and the help screen.
std::string_view perkDescription(PerkType type);

/// Which tiers the perk appears in, as text ("all tiers", "tier 2+", ...).
std::string_view perkTierText(PerkType type);

/// True if @p type may spawn in @p tier.
bool isPerkAllowedInTier(PerkType type, Tier tier);

/// Number of perks placed in an N x N maze of @p tier (about N/8 plus a tier bonus).
int perkCountFor(int mazeSize, Tier tier);

/// Places perks on random reachable cells. Never on the start cell, its neighbours or the exit,
/// and never two on the same cell. Deterministic for a given generator state.
std::vector<PerkItem> spawnPerks(const Maze& maze, Tier tier, Rng& rng);

/// Timed effects. Phase comes from Magic Berries, the others directly from pickups.
enum class EffectType : std::uint8_t { Phase, TimeFreeze, CompassTrail, Lantern, ChaserFreeze };

inline constexpr int EFFECT_TYPE_COUNT = 5;

inline constexpr std::array<EffectType, EFFECT_TYPE_COUNT> ALL_EFFECT_TYPES = {
    EffectType::Phase, EffectType::TimeFreeze, EffectType::CompassTrail, EffectType::Lantern,
    EffectType::ChaserFreeze};

/// Display name of an effect ("Phase", "Time frozen", ...).
std::string_view effectName(EffectType type);

/// Full duration of an effect in seconds.
float effectDuration(EffectType type);

/// Key used in save files ("phase", "timefreeze", ...).
std::string_view effectKey(EffectType type);

/// Remaining time of every effect.
class EffectTimers {
public:
    /// Starts (or refreshes) @p type with its full duration.
    void start(EffectType type);

    /// Sets the remaining time directly, clamped to [0, duration] (used by snapshots).
    void setRemaining(EffectType type, float seconds);

    /// Seconds left (0 when inactive).
    float remaining(EffectType type) const;

    /// True while the effect is running.
    bool isActive(EffectType type) const { return remaining(type) > 0.0f; }

    /// Counts every effect down; returns the effects that ended during this update.
    std::vector<EffectType> update(float deltaSeconds);

    /// Stops all effects.
    void clear() { m_remaining.fill(0.0f); }

private:
    std::array<float, EFFECT_TYPE_COUNT> m_remaining{};
};

} // namespace core
