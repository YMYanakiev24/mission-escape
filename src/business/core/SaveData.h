#pragma once

#include "core/Config.h"
#include "core/LevelRules.h"
#include "core/LevelSnapshot.h"
#include "core/Progress.h"

#include <optional>
#include <string>

/// @file SaveData.h
/// Everything stored in one save slot.

namespace core {

/// Contents of one save slot (a player profile).
struct SaveData {
    std::string playerName;
    int mazeSize = DEFAULT_MAZE_SIZE;
    ViewMode viewMode = ViewMode::Classic2D;
    std::string lastPlayed; ///< "YYYY-MM-DD HH:MM", informational only.
    Progress progress;
    std::optional<LevelSnapshot> snapshot; ///< Level left with "Save & Exit", if any.
};

} // namespace core
