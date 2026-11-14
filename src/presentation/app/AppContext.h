#pragma once

#include "app/Navigator.h"
#include "app/UiSounds.h"
#include "audio/AudioManager.h"
#include "core/Config.h"
#include "core/LevelRules.h"
#include "core/ProfileService.h"
#include "core/Progress.h"
#include "core/Repositories.h"
#include "core/SaveData.h"
#include "core/Settings.h"
#include "render/MenuBackground.h"
#include "render/Theme.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

/// @file AppContext.h
/// Services shared by all screens: fonts, navigation, profiles, settings, the game session.
/// Profiles and settings come from the business layer (core::ProfileService); this layer never
/// touches the data layer directly.

namespace core {
class Level;
} // namespace core

namespace app {

class GameSession;

/// Options parsed from the command line.
struct LaunchOptions {
    bool screenshotTour = false; ///< `--screenshots`: auto-walk the screens, save PNGs, exit.
};

/// Summary of the most recently finished level, shown by the result screens.
struct LevelOutcome {
    int level = 1;
    int stars = 0;
    float seconds = 0.0f;
    float parSeconds = 0.0f;
    int steps = 0;         ///< Moves the player made.
    int shortestSteps = 0; ///< Door-free shortest route from start to exit.
    bool newRecord = false;
    int bestStars = 0;
    float bestSeconds = 0.0f;
    std::optional<core::SkinId> unlockedSkin;
};

/// Everything screens share (a plain service bundle, hence a struct with public members).
/// Owned by App and destroyed before the window closes, so GPU resources held here are
/// always released while the GL context still exists.
struct AppContext {
    /// Uses @p saves and @p settingsStore, the data-layer implementations chosen by main(),
    /// only through their business-layer interfaces.
    AppContext(LaunchOptions launchOptions, core::SaveRepository& saves,
               core::SettingsRepository& settingsStore);
    /// Releases the session and GPU resources (before the window closes).
    ~AppContext();
    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;

    /// Prepares @p level of the active profile with a fresh random seed and switches to the
    /// gameplay screen. The new session replaces the old one when the fade reaches black.
    void startLevel(int level);

    /// Prepares the current level again with the same seed and switches to gameplay.
    void retryLevel();

    /// Restarts the current level immediately with the same seed (used by the pause menu).
    void restartLevel();

    /// Schedules the running session to be discarded at the next screen switch.
    void endSession();

    /// Applies a scheduled session change; called by the screen stack at the black frame.
    void applyPendingSessionChange();

    /// Records a finished level in the active profile and fills lastOutcome.
    void recordLevelResult(const core::Level& level);

    /// Resumes the in-progress level stored in the active profile; false if there is none
    /// or it no longer matches its maze (it is then discarded).
    bool resumeSnapshot();

    /// Stores the running level as a snapshot in the active profile, saves, and returns to
    /// the main menu.
    void saveAndExitSession();

    /// Applies display settings (fullscreen) to the window.
    void applyDisplaySettings();

    LaunchOptions options;
    core::ProfileService profiles; ///< Save slots, active profile and settings (business layer).
    render::Theme theme;
    audio::AudioManager audio;
    UiSounds uiSounds{audio}; ///< Hover/click sounds for every menu.
    render::MenuBackground menuBackground;
    Navigator navigator;
    std::unique_ptr<GameSession> session;        ///< The level being played, if any.
    std::unique_ptr<GameSession> pendingSession; ///< Replaces session at the next switch.
    bool pendingSessionEnd = false;              ///< Discard session at the next switch.
    LevelOutcome lastOutcome;
    float time = 0.0f; ///< Global animation clock in seconds.
};

} // namespace app
