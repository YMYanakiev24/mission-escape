#pragma once

#include "app/Navigator.h"
#include "app/UiSounds.h"
#include "audio/AudioManager.h"
#include "core/Config.h"
#include "core/LevelRules.h"
#include "core/Progress.h"
#include "core/SaveData.h"
#include "core/Settings.h"
#include "io/SaveSystem.h"
#include "io/SettingsStore.h"
#include "render/MenuBackground.h"
#include "render/Theme.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

/// @file AppContext.h
/// Services shared by all screens: fonts, navigation, profiles, settings, the game session.

namespace core {
class Level;
} // namespace core

namespace app {

class GameSession;

/// Options parsed from the command line.
struct LaunchOptions {
    bool screenshotTour = false; ///< `--screenshots`: auto-walk the screens, save PNGs, exit.
};

/// State of one save slot as seen by the menus.
enum class SlotStatus { Empty, Ready, Corrupt };

/// One of the three save slots.
struct SaveSlot {
    SlotStatus status = SlotStatus::Empty;
    std::optional<core::SaveData> data; ///< Present when status is Ready.
    std::string error;                  ///< Reason when status is Corrupt.
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
    /// @param dataDirectory folder that holds `saves/` and `settings.cfg`.
    AppContext(LaunchOptions launchOptions, const std::filesystem::path& dataDirectory);
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

    /// The profile being played, or null when no slot is active.
    core::SaveData* activeProfile();

    /// Makes @p slot (1..3) the active profile; returns false if it holds no valid data.
    bool activateSlot(int slot);

    /// Stores @p data in @p slot and makes it the active profile.
    void createProfile(int slot, core::SaveData data);

    /// Records a finished level in the active profile and fills lastOutcome.
    void recordLevelResult(const core::Level& level);

    /// Writes the active profile to its slot (stamping the last-played time).
    void saveActiveProfile();

    /// Writes settings.cfg.
    void saveSettings();

    /// Reads settings.cfg and all three save slots from disk.
    void loadPersistentData();

    /// Deletes the save file of @p slot and forgets it in memory.
    void deleteSlot(int slot);

    /// Resumes the in-progress level stored in the active profile; false if there is none
    /// or it no longer matches its maze (it is then discarded).
    bool resumeSnapshot();

    /// Stores the running level as a snapshot in the active profile, saves, and returns to
    /// the main menu.
    void saveAndExitSession();

    /// Applies display settings (fullscreen) to the window.
    void applyDisplaySettings();

    LaunchOptions options;
    bool persistenceEnabled = true; ///< False during the screenshot tour.
    io::SaveSystem saves;
    io::SettingsStore settingsStore;
    std::string lastPersistenceError; ///< Shown as a toast when a save fails.
    render::Theme theme;
    audio::AudioManager audio;
    UiSounds uiSounds{audio}; ///< Hover/click sounds for every menu.
    render::MenuBackground menuBackground;
    Navigator navigator;
    core::Settings settings;
    std::array<SaveSlot, core::SAVE_SLOT_COUNT> slots;
    int activeSlot = 0;                          ///< 1..3, or 0 when no profile is active.
    std::unique_ptr<GameSession> session;        ///< The level being played, if any.
    std::unique_ptr<GameSession> pendingSession; ///< Replaces session at the next switch.
    bool pendingSessionEnd = false;              ///< Discard session at the next switch.
    LevelOutcome lastOutcome;
    float time = 0.0f; ///< Global animation clock in seconds.
};

} // namespace app
