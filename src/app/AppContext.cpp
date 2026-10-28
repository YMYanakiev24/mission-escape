#include "app/AppContext.h"

#include "app/GameSession.h"
#include "core/Random.h"
#include "core/Scoring.h"
#include "core/Validation.h"

#include <raylib.h>

#include <utility>

namespace app {

namespace {

constexpr float MILLISECONDS_TO_SECONDS = 0.001f;
constexpr const char* SAVE_FOLDER = "saves";
constexpr const char* SETTINGS_FILE = "settings.cfg";

SaveSlot slotFromLoadResult(io::LoadResult result) {
    SaveSlot slot;
    switch (result.status) {
        case io::LoadStatus::Ok:
            slot.status = SlotStatus::Ready;
            slot.data = std::move(result.data);
            break;
        case io::LoadStatus::Missing:
            slot.status = SlotStatus::Empty;
            break;
        case io::LoadStatus::Corrupt:
            slot.status = SlotStatus::Corrupt;
            slot.error = result.error;
            break;
    }
    return slot;
}

} // namespace

AppContext::AppContext(LaunchOptions launchOptions, const std::filesystem::path& dataDirectory)
    : options(launchOptions), persistenceEnabled(!launchOptions.screenshotTour),
      saves(dataDirectory / SAVE_FOLDER), settingsStore(dataDirectory / SETTINGS_FILE) {}

// Defined here, where GameSession is a complete type, so unique_ptr can destroy it.
AppContext::~AppContext() = default;

void AppContext::startLevel(int level) {
    core::SaveData* profile = activeProfile();
    const int size = profile != nullptr ? profile->mazeSize : core::DEFAULT_MAZE_SIZE;
    const core::ViewMode mode = profile != nullptr ? profile->viewMode : core::ViewMode::Classic2D;
    if (profile != nullptr && profile->snapshot) {
        profile->snapshot.reset(); // starting another attempt replaces the saved one
        saveActiveProfile();
    }
    pendingSession =
        std::make_unique<GameSession>(*this, level, size, core::makeRandomSeed(), mode);
    pendingSessionEnd = false;
    navigator.replaceWith(ScreenId::Gameplay);
}

void AppContext::retryLevel() {
    if (!session) {
        return;
    }
    const core::Level& level = session->level();
    pendingSession = std::make_unique<GameSession>(*this, level.levelNumber(), session->mazeSize(),
                                                   level.seed(), level.viewMode());
    pendingSessionEnd = false;
    navigator.replaceWith(ScreenId::Gameplay);
}

void AppContext::restartLevel() {
    if (session) {
        session->restart();
    }
}

void AppContext::endSession() {
    pendingSession.reset();
    pendingSessionEnd = true;
}

void AppContext::applyPendingSessionChange() {
    if (pendingSession) {
        session = std::move(pendingSession);
    } else if (pendingSessionEnd) {
        session.reset();
    }
    pendingSessionEnd = false;
}

core::SaveData* AppContext::activeProfile() {
    if (!core::isValidSlot(activeSlot)) {
        return nullptr;
    }
    SaveSlot& slot = slots[static_cast<std::size_t>(activeSlot - 1)];
    return slot.status == SlotStatus::Ready && slot.data ? &*slot.data : nullptr;
}

bool AppContext::activateSlot(int slot) {
    if (!core::isValidSlot(slot) ||
        slots[static_cast<std::size_t>(slot - 1)].status != SlotStatus::Ready) {
        return false;
    }
    activeSlot = slot;
    settings.lastSlot = slot;
    saveSettings();
    return true;
}

void AppContext::createProfile(int slot, core::SaveData data) {
    if (!core::isValidSlot(slot)) {
        return;
    }
    SaveSlot& target = slots[static_cast<std::size_t>(slot - 1)];
    target.status = SlotStatus::Ready;
    target.data = std::move(data);
    target.error.clear();
    activateSlot(slot);
    saveActiveProfile();
}

void AppContext::recordLevelResult(int level, int stars, float seconds, float parSeconds) {
    LevelOutcome outcome;
    outcome.level = level;
    outcome.stars = stars;
    outcome.seconds = seconds;
    outcome.parSeconds = parSeconds;
    if (core::SaveData* profile = activeProfile()) {
        const core::ProgressUpdate update =
            profile->progress.applyResult(level, stars, core::toMilliseconds(seconds));
        const core::LevelRecord& record = profile->progress.record(level);
        outcome.newRecord = update.newRecord;
        outcome.unlockedSkin = update.skinUnlocked;
        outcome.bestStars = record.bestStars;
        outcome.bestSeconds = static_cast<float>(record.bestTimeMs) * MILLISECONDS_TO_SECONDS;
        profile->snapshot.reset();
        saveActiveProfile(); // autosave after every completed level
    }
    lastOutcome = outcome;
}

void AppContext::saveActiveProfile() {
    core::SaveData* profile = activeProfile();
    if (profile == nullptr || !persistenceEnabled) {
        return;
    }
    profile->lastPlayed = io::currentTimestamp();
    std::string error;
    if (!saves.save(activeSlot, *profile, error)) {
        lastPersistenceError = "Could not save: " + error;
    }
}

void AppContext::saveSettings() {
    if (!persistenceEnabled) {
        return;
    }
    std::string error;
    if (!settingsStore.save(settings, error)) {
        lastPersistenceError = "Could not save settings: " + error;
    }
}

void AppContext::loadPersistentData() {
    if (!persistenceEnabled) {
        return;
    }
    settings = settingsStore.load();
    for (int slot = 1; slot <= core::SAVE_SLOT_COUNT; ++slot) {
        slots[static_cast<std::size_t>(slot - 1)] = slotFromLoadResult(saves.load(slot));
    }
}

void AppContext::deleteSlot(int slot) {
    if (!core::isValidSlot(slot)) {
        return;
    }
    if (persistenceEnabled) {
        saves.erase(slot);
    }
    slots[static_cast<std::size_t>(slot - 1)] = SaveSlot{};
    if (activeSlot == slot) {
        activeSlot = 0;
    }
    if (settings.lastSlot == slot) {
        settings.lastSlot = 0;
        saveSettings();
    }
}

bool AppContext::resumeSnapshot() {
    core::SaveData* profile = activeProfile();
    if (profile == nullptr || !profile->snapshot) {
        return false;
    }
    const core::LevelSnapshot& saved = *profile->snapshot;
    auto resumed = std::make_unique<GameSession>(*this, saved.level, profile->mazeSize, saved.seed,
                                                 profile->viewMode);
    if (!resumed->restoreSnapshot(saved)) {
        profile->snapshot.reset(); // does not fit its maze any more: drop it instead of crashing
        saveActiveProfile();
        lastPersistenceError = "The saved level could not be resumed and was discarded.";
        return false;
    }
    pendingSession = std::move(resumed);
    pendingSessionEnd = false;
    navigator.replaceWith(ScreenId::Gameplay);
    return true;
}

void AppContext::saveAndExitSession() {
    core::SaveData* profile = activeProfile();
    if (session && profile != nullptr && !session->level().isFinished()) {
        profile->snapshot = session->level().snapshot();
        saveActiveProfile();
    }
    endSession();
    navigator.replaceWith(ScreenId::MainMenu);
}

void AppContext::applyDisplaySettings() {
    const bool borderless = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
    if (settings.fullscreen != borderless) {
        ToggleBorderlessWindowed();
    }
}

} // namespace app
