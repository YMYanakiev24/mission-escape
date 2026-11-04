#include "core/LevelRules.h"

#include "core/Config.h"

#include <algorithm>

namespace core {

bool isValidLevel(int level) {
    return level >= 1 && level <= LEVEL_COUNT;
}

Tier tierForLevel(int level) {
    const int clamped = std::clamp(level, 1, LEVEL_COUNT);
    return static_cast<Tier>((clamped - 1) / LEVELS_PER_TIER);
}

int tierNumber(Tier tier) {
    return static_cast<int>(tier) + 1;
}

int firstLevelOfTier(Tier tier) {
    return static_cast<int>(tier) * LEVELS_PER_TIER + 1;
}

std::string_view tierName(Tier tier) {
    switch (tier) {
        case Tier::Awakening:
            return "Awakening";
        case Tier::Labyrinth:
            return "Labyrinth";
        case Tier::Fog:
            return "Fog";
        case Tier::Hunted:
            return "Hunted";
    }
    return "Awakening";
}

std::string_view tierDescription(Tier tier) {
    switch (tier) {
        case Tier::Awakening:
            return "Classic perfect mazes: exactly one route leads from the start to the exit.";
        case Tier::Labyrinth:
            return "Longer routes, decoy branches near the exit and stricter star times. "
                   "Look for green runes: secret doors that shorten the way.";
        case Tier::Fog:
            return "Fog of war. You only see a few cells around you and remember where you "
                   "have been. The exit glows faintly through the dark.";
        case Tier::Hunted:
            return "The maze has loops, but something hunts you. Keep moving, use Frost Shards, "
                   "and slip through secret doors it cannot follow.";
    }
    return "";
}

LevelSpec levelSpec(int level) {
    LevelSpec spec;
    spec.level = std::clamp(level, 1, LEVEL_COUNT);
    spec.tier = tierForLevel(spec.level);
    if (spec.tier != Tier::Awakening) {
        spec.secretDoorMin = SECRET_DOOR_MIN_COUNT;
        spec.secretDoorMax = SECRET_DOOR_MAX_COUNT;
    }
    switch (spec.tier) {
        case Tier::Awakening:
            break;
        case Tier::Labyrinth:
            spec.candidateCount = LABYRINTH_CANDIDATE_COUNT;
            spec.exitDecoys = true;
            break;
        case Tier::Fog:
            spec.hasFog = true;
            break;
        case Tier::Hunted: {
            spec.braidRatio = BRAID_RATIO;
            spec.hasChaser = true;
            const float progress = static_cast<float>(spec.level - firstLevelOfTier(spec.tier)) /
                                   static_cast<float>(LEVELS_PER_TIER - 1);
            spec.chaserBaseSpeed = CHASER_SPEED_FIRST_LEVEL +
                                   (CHASER_SPEED_LAST_LEVEL - CHASER_SPEED_FIRST_LEVEL) * progress;
            break;
        }
    }
    return spec;
}

std::string_view viewModeKey(ViewMode mode) {
    return mode == ViewMode::FirstPerson3D ? "3d" : "2d";
}

std::string_view viewModeName(ViewMode mode) {
    return mode == ViewMode::FirstPerson3D ? "3D First-Person" : "2D Classic";
}

float moveSecondsFor(ViewMode mode) {
    return mode == ViewMode::FirstPerson3D ? MOVE_SECONDS_3D : MOVE_SECONDS_2D;
}

} // namespace core
