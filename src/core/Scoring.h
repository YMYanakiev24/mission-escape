#pragma once

#include "core/LevelRules.h"

#include <string>

/// @file Scoring.h
/// Pure functions for par times, star ratings and personal records.

namespace core {

/// Best result achieved on one level.
struct LevelRecord {
    int bestStars = 0;      ///< 0 = never completed.
    int bestTimeMs = 0;     ///< Fastest completion in milliseconds (0 = none yet).
    bool completed = false; ///< True once the level was finished with at least one star.
};

/// What the HUD shows as the next star goal, e.g. "3 stars under 0:42".
struct StarTarget {
    int stars = 1;            ///< Best rating still reachable.
    float secondsLeft = 0.0f; ///< Seconds left to keep that rating (0 when stars == 1).
};

/// Time multiplier of a tier (fog levels are slower, labyrinth levels stricter).
float tierTimeMultiplier(Tier tier);

/// Time multiplier of a view mode (3D navigation is slower).
float modeTimeMultiplier(ViewMode mode);

/// parTime = shortestPathLength * SECONDS_PER_CELL * tierMultiplier * modeMultiplier.
float parTime(int shortestPathLength, Tier tier, ViewMode mode);

/// Latest completion time that still earns @p stars (3 or 2); infinite for 1 star.
float starTimeLimit(int stars, float par);

/// Star rating (1-3) for finishing a level in @p seconds; a time exactly on a threshold
/// still earns the better rating.
int starsForTime(float seconds, float par);

/// The best rating still reachable at @p elapsed seconds and how long it remains reachable.
StarTarget starTargetAt(float elapsed, float par);

/// Converts seconds to whole milliseconds (negative values become 0).
int toMilliseconds(float seconds);

/// Updates @p record with a finished attempt. Attempts with 0 stars (quit) never change it.
/// @return true when the record improved (more stars or a faster time).
bool updateRecord(LevelRecord& record, int stars, int timeMs);

/// Formats seconds as "m:ss.t" (e.g. "1:05.3"); used by the HUD and result screens.
std::string formatDuration(float seconds);

} // namespace core
