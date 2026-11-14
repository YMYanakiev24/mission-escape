#include "app/AppContext.h"

#include "app/GameSession.h"
#include "core/Random.h"
#include "core/Scoring.h"

#include <raylib.h>

#include <utility>

namespace app {

namespace {

constexpr float MILLISECONDS_TO_SECONDS = 0.001f;

} // namespace

AppContext::AppContext(LaunchOptions launchOptions, core::SaveRepository& saves,
                       core::SettingsRepository& settingsStore)
    : options(launchOptions), profiles(saves, settingsStore, !launchOptions.screenshotTour) {}

// Defined here, where GameSession is a complete type, so unique_ptr can destroy it.
AppContext::~AppContext() = default;

void AppContext::startLevel(int level) {
    const core::SaveData* profile = profiles.activeProfile();
    const int size = profile != nullptr ? profile->mazeSize : core::DEFAULT_MAZE_SIZE;
    const core::ViewMode mode = profile != nullptr ? profile->viewMode : core::ViewMode::Classic2D;
    profiles.discardSavedAttempt(); // starting another attempt replaces the saved one
    pendingSession =
        std::make_unique<GameSession>(*this, level, size, core::makeRandomSeed(), mode);
    pendingSessionEnd = false;
    navigator.replaceWith(ScreenId::Gameplay);
}

void AppContext::retryLevel() {
    if (!session) {
        return;
    }
    profiles.discardSavedAttempt();
    const core::Level& level = session->level();
    pendingSession = std::make_unique<GameSession>(*this, level.levelNumber(), session->mazeSize(),
                                                   level.seed(), level.viewMode());
    pendingSessionEnd = false;
    navigator.replaceWith(ScreenId::Gameplay);
}

void AppContext::restartLevel() {
    if (session) {
        profiles.discardSavedAttempt();
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

void AppContext::recordLevelResult(const core::Level& level) {
    LevelOutcome outcome;
    outcome.level = level.levelNumber();
    outcome.stars = level.earnedStars();
    outcome.seconds = level.elapsedSeconds();
    outcome.parSeconds = level.parTimeSeconds();
    outcome.steps = level.stepsTaken();
    outcome.shortestSteps = level.shortestPathLength();
    const std::optional<core::RecordedResult> recorded =
        profiles.recordResult(outcome.level, outcome.stars, core::toMilliseconds(outcome.seconds));
    if (recorded) {
        outcome.newRecord = recorded->update.newRecord;
        outcome.unlockedSkin = recorded->update.skinUnlocked;
        outcome.bestStars = recorded->record.bestStars;
        outcome.bestSeconds =
            static_cast<float>(recorded->record.bestTimeMs) * MILLISECONDS_TO_SECONDS;
    }
    lastOutcome = outcome;
}

bool AppContext::resumeSnapshot() {
    const core::SaveData* profile = profiles.activeProfile();
    if (profile == nullptr || !profile->snapshot) {
        return false;
    }
    const core::LevelSnapshot& saved = *profile->snapshot;
    auto resumed = std::make_unique<GameSession>(*this, saved.level, profile->mazeSize, saved.seed,
                                                 profile->viewMode);
    if (!resumed->restoreSnapshot(saved)) {
        // It does not fit its maze any more: drop it instead of crashing.
        profiles.discardSavedAttempt();
        profiles.lastError() = "The saved level could not be resumed and was discarded.";
        return false;
    }
    pendingSession = std::move(resumed);
    pendingSessionEnd = false;
    navigator.replaceWith(ScreenId::Gameplay);
    return true;
}

void AppContext::saveAndExitSession() {
    if (session && !session->level().isFinished()) {
        profiles.storeSnapshot(session->level().snapshot());
    }
    endSession();
    navigator.replaceWith(ScreenId::MainMenu);
}

void AppContext::applyDisplaySettings() {
    const bool borderless = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
    if (profiles.settings().fullscreen != borderless) {
        ToggleBorderlessWindowed();
    }
}

} // namespace app
