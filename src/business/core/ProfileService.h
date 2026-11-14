#pragma once

#include "core/Config.h"
#include "core/LevelSnapshot.h"
#include "core/Progress.h"
#include "core/Repositories.h"
#include "core/SaveData.h"
#include "core/Scoring.h"
#include "core/Settings.h"

#include <array>
#include <optional>
#include <string>

/// @file ProfileService.h
/// Business logic of the save slots and the settings: which profile is active, what a finished
/// level changes and when everything is written. Storage goes through the repository
/// interfaces, so this class never touches a file.

namespace core {

/// State of one save slot as seen by the menus.
enum class SlotStatus { Empty, Ready, Corrupt };

/// One of the three save slots.
struct SaveSlot {
    SlotStatus status = SlotStatus::Empty;
    std::optional<SaveData> data; ///< Present when status is Ready.
    std::string error;            ///< Reason when status is Corrupt.
};

/// What recording a finished level changed in the active profile.
struct RecordedResult {
    ProgressUpdate update; ///< New record, unlocked level or skin.
    LevelRecord record;    ///< The level's record after the update.
};

/// Current local date and time as "YYYY-MM-DD HH:MM" (stamped into saved profiles).
std::string currentTimestamp();

/// Manages the three save slots, the active profile and the settings.
class ProfileService {
public:
    /// Reads and writes through @p saves and @p settingsStore. With @p persistenceEnabled false
    /// nothing is read or written (the screenshot tour plays with an in-memory profile).
    ProfileService(SaveRepository& saves, SettingsRepository& settingsStore,
                   bool persistenceEnabled);

    ProfileService(const ProfileService&) = delete;
    ProfileService& operator=(const ProfileService&) = delete;

    /// Reads the settings and all three save slots.
    void loadAll();

    /// The user settings (changes are kept in memory until saveSettings()).
    Settings& settings() noexcept { return m_settings; }
    /// The user settings.
    const Settings& settings() const noexcept { return m_settings; }

    /// The three save slots.
    std::array<SaveSlot, SAVE_SLOT_COUNT>& slots() noexcept { return m_slots; }
    /// The three save slots.
    const std::array<SaveSlot, SAVE_SLOT_COUNT>& slots() const noexcept { return m_slots; }

    /// The active slot 1..3, or 0 when no profile is active.
    int activeSlot() const noexcept { return m_activeSlot; }

    /// The profile being played, or null when no slot is active.
    SaveData* activeProfile();

    /// Makes @p slot the active profile and remembers it for "Continue"; returns false if the
    /// slot holds no valid profile.
    bool activateSlot(int slot);

    /// Stores @p data in @p slot, makes it the active profile and saves it.
    void createProfile(int slot, SaveData data);

    /// Deletes @p slot from storage and memory (and forgets it if it was active or "Continue").
    void deleteSlot(int slot);

    /// Applies a finished level to the active profile, drops its saved attempt and saves.
    /// Returns nothing when no profile is active.
    std::optional<RecordedResult> recordResult(int level, int stars, int timeMs);

    /// Stores @p snapshot as the active profile's saved attempt ("Save & Exit") and saves.
    void storeSnapshot(const LevelSnapshot& snapshot);

    /// Drops the active profile's saved attempt, and saves, if it has one.
    void discardSavedAttempt();

    /// Writes the active profile, stamping the last-played time. A failure is reported
    /// through lastError().
    void saveActiveProfile();

    /// Writes the settings. A failure is reported through lastError().
    void saveSettings();

    /// The most recent storage error; the user interface shows it once and clears it.
    std::string& lastError() noexcept { return m_lastError; }

private:
    SaveRepository& m_saves;
    SettingsRepository& m_settingsStore;
    bool m_persistenceEnabled;
    Settings m_settings;
    std::array<SaveSlot, SAVE_SLOT_COUNT> m_slots{};
    int m_activeSlot = 0;
    std::string m_lastError;
};

} // namespace core
