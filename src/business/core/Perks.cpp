#include "core/Perks.h"

#include "core/Config.h"
#include "core/Pathfinding.h"

#include <algorithm>

namespace core {

namespace {

std::size_t effectIndex(EffectType type) {
    return static_cast<std::size_t>(type);
}

int tierPerkBonus(Tier tier) {
    switch (tier) {
        case Tier::Awakening:
            return PERK_BONUS_AWAKENING;
        case Tier::Labyrinth:
            return PERK_BONUS_LABYRINTH;
        case Tier::Fog:
            return PERK_BONUS_FOG;
        case Tier::Hunted:
            return PERK_BONUS_HUNTED;
    }
    return 0;
}

/// Cells where a perk may appear: reachable, not the start, its neighbours or the exit.
std::vector<Cell> perkCandidateCells(const Maze& maze) {
    const Grid2D<int> distances = bfsDistances(maze, maze.start(), PassRules{});
    std::vector<Cell> cells;
    for (int y = 0; y < maze.size(); ++y) {
        for (int x = 0; x < maze.size(); ++x) {
            const Cell cell{x, y};
            const bool reachable = distances[y][x] != UNREACHABLE;
            const bool nearStart = manhattanDistance(cell, maze.start()) <= 1;
            if (reachable && !nearStart && cell != maze.exit()) {
                cells.push_back(cell);
            }
        }
    }
    return cells;
}

/// The perk types of a tier, with the Magic Berry and the tier's signature perk guaranteed
/// first so even small mazes get them.
std::vector<PerkType> perkTypesFor(int count, Tier tier, Rng& rng) {
    std::vector<PerkType> allowed;
    for (const PerkType type : ALL_PERK_TYPES) {
        if (isPerkAllowedInTier(type, tier)) {
            allowed.push_back(type);
        }
    }
    std::vector<PerkType> types;
    types.push_back(PerkType::MagicBerry);
    if (tier == Tier::Fog) {
        types.push_back(PerkType::Lantern);
    } else if (tier == Tier::Hunted) {
        types.push_back(PerkType::FrostShard);
    }
    while (static_cast<int>(types.size()) < count) {
        types.push_back(allowed[static_cast<std::size_t>(
            rng.nextInt(0, static_cast<int>(allowed.size()) - 1))]);
    }
    types.resize(static_cast<std::size_t>(count));
    return types;
}

} // namespace

std::string_view perkName(PerkType type) {
    switch (type) {
        case PerkType::MagicBerry:
            return "Magic Berry";
        case PerkType::Hourglass:
            return "Hourglass";
        case PerkType::CompassSpark:
            return "Compass Spark";
        case PerkType::Lantern:
            return "Lantern";
        case PerkType::FrostShard:
            return "Frost Shard";
    }
    return "Perk";
}

std::string_view perkDescription(PerkType type) {
    switch (type) {
        case PerkType::MagicBerry:
            return "Stored (max 3). Press Shift to phase through inner walls for 3 seconds.";
        case PerkType::Hourglass:
            return "Freezes the level timer for 5 seconds.";
        case PerkType::CompassSpark:
            return "Lights the shortest path to the exit for 3 seconds.";
        case PerkType::Lantern:
            return "Doubles your sight radius in the fog for 10 seconds.";
        case PerkType::FrostShard:
            return "Freezes the chaser in place for 4 seconds.";
    }
    return "";
}

std::string_view perkTierText(PerkType type) {
    switch (type) {
        case PerkType::MagicBerry:
        case PerkType::Hourglass:
            return "all tiers";
        case PerkType::CompassSpark:
            return "tier 2 and up";
        case PerkType::Lantern:
            return "tier 3 (Fog)";
        case PerkType::FrostShard:
            return "tier 4 (Hunted)";
    }
    return "";
}

bool isPerkAllowedInTier(PerkType type, Tier tier) {
    switch (type) {
        case PerkType::MagicBerry:
        case PerkType::Hourglass:
            return true;
        case PerkType::CompassSpark:
            return tier != Tier::Awakening;
        case PerkType::Lantern:
            return tier == Tier::Fog;
        case PerkType::FrostShard:
            return tier == Tier::Hunted;
    }
    return false;
}

int perkCountFor(int mazeSize, Tier tier) {
    return std::max(1, mazeSize / PERK_SIZE_DIVISOR) + tierPerkBonus(tier);
}

std::vector<PerkItem> spawnPerks(const Maze& maze, Tier tier, Rng& rng) {
    std::vector<Cell> cells = perkCandidateCells(maze);
    rng.shuffle(cells);
    const int count = std::min(perkCountFor(maze.size(), tier), static_cast<int>(cells.size()));
    const std::vector<PerkType> types = perkTypesFor(count, tier, rng);
    std::vector<PerkItem> perks;
    for (int index = 0; index < count; ++index) {
        const auto position = static_cast<std::size_t>(index);
        perks.push_back(PerkItem{index, types[position], cells[position], false});
    }
    return perks;
}

std::string_view effectName(EffectType type) {
    switch (type) {
        case EffectType::Phase:
            return "Phase";
        case EffectType::TimeFreeze:
            return "Time frozen";
        case EffectType::CompassTrail:
            return "Compass trail";
        case EffectType::Lantern:
            return "Lantern";
        case EffectType::ChaserFreeze:
            return "Chaser frozen";
    }
    return "";
}

float effectDuration(EffectType type) {
    switch (type) {
        case EffectType::Phase:
            return PHASE_SECONDS;
        case EffectType::TimeFreeze:
            return HOURGLASS_SECONDS;
        case EffectType::CompassTrail:
            return COMPASS_SECONDS;
        case EffectType::Lantern:
            return LANTERN_SECONDS;
        case EffectType::ChaserFreeze:
            return FROST_SECONDS;
    }
    return 0.0f;
}

std::string_view effectKey(EffectType type) {
    switch (type) {
        case EffectType::Phase:
            return "phase";
        case EffectType::TimeFreeze:
            return "timefreeze";
        case EffectType::CompassTrail:
            return "compass";
        case EffectType::Lantern:
            return "lantern";
        case EffectType::ChaserFreeze:
            return "chaserfreeze";
    }
    return "";
}

void EffectTimers::start(EffectType type) {
    m_remaining[effectIndex(type)] = effectDuration(type);
}

void EffectTimers::setRemaining(EffectType type, float seconds) {
    m_remaining[effectIndex(type)] = std::clamp(seconds, 0.0f, effectDuration(type));
}

float EffectTimers::remaining(EffectType type) const {
    return m_remaining[effectIndex(type)];
}

std::vector<EffectType> EffectTimers::update(float deltaSeconds) {
    std::vector<EffectType> ended;
    for (const EffectType type : ALL_EFFECT_TYPES) {
        float& left = m_remaining[effectIndex(type)];
        if (left <= 0.0f) {
            continue;
        }
        left = std::max(0.0f, left - deltaSeconds);
        if (left <= 0.0f) {
            ended.push_back(type);
        }
    }
    return ended;
}

} // namespace core
