#include "app/ScreenshotTour.h"

#include "app/GameSession.h"
#include "core/Pathfinding.h"
#include "ui/GameplayScreen.h"

#include <raylib.h>
#include <rlgl.h>

#include <array>
#include <filesystem>
#include <memory>
#include <system_error>
#include <utility>

namespace app {

namespace {

constexpr const char* OUTPUT_DIRECTORY = "screenshots";
constexpr int SETTLE_FRAMES = 45;
constexpr int WALK_INTERVAL_FRAMES = 9;
constexpr float INSTANT_STEP_SECONDS = 0.05f;
constexpr int INSTANT_STEP_LIMIT = 100000;
constexpr std::uint32_t DEMO_SEED = 20261018U;
constexpr float SIMULATION_STEP = 1.0f / 60.0f;
constexpr float SIMULATION_LIMIT_SECONDS = 120.0f;
/// Every tier intro seen except the Fog one (bit 2), so the tour can capture that card.
constexpr std::uint32_t FOG_UNSEEN_INTRO_MASK = 0b1011U;
constexpr std::uint32_t ALL_SKINS_MASK = 0b11111U;
/// Frame of a 3D step at which the walker stops and looks down the longest corridor.
constexpr int FIRST_PERSON_LOOK_FRAME = 120;

/// First step of the player's shortest route to the exit, if any.
std::optional<core::Direction> nextStepToExit(const core::Level& level) {
    return core::nextStepToward(level.maze(), level.player().cell(), level.maze().exit(),
                                core::PassRules{true, false});
}

/// In 3D, moves do not turn the player, so the tour turns towards each step first.
void faceTowards(core::Level& level, core::Direction direction) {
    for (int turn = 0; turn < 4 && level.player().facing() != direction; ++turn) {
        level.turnRight();
    }
}

/// Turns the player towards the longest straight open corridor (nicer 3D captures).
void faceLongestCorridor(core::Level& level) {
    core::Direction best = level.player().facing();
    int bestLength = -1;
    for (const core::Direction direction : core::ALL_DIRECTIONS) {
        int length = 0;
        for (core::Cell cell = level.player().cell(); level.maze().isOpen(cell, direction);
             cell = core::neighbour(cell, direction)) {
            ++length;
        }
        if (length > bestLength) {
            best = direction;
            bestLength = length;
        }
    }
    faceTowards(level, best);
}

/// Advances the level by @p seconds in small steps (the player stands still).
void simulate(core::Level& level, float seconds) {
    for (float time = 0.0f; time < seconds && !level.isFinished(); time += SIMULATION_STEP) {
        level.update(SIMULATION_STEP);
    }
}

/// Walks @p steps along the shortest path to the exit, simulating each move.
void walkInstantly(core::Level& level, int steps) {
    for (int step = 0; step < steps && !level.isFinished(); ++step) {
        const std::optional<core::Direction> direction = nextStepToExit(level);
        if (!direction) {
            return;
        }
        faceTowards(level, *direction);
        level.tryMove(*direction);
        while (level.player().isMoving() && !level.isFinished()) {
            level.update(SIMULATION_STEP);
        }
    }
}

/// Plays the rest of the level instantly by following the shortest path.
void finishLevelInstantly(core::Level& level) {
    for (int step = 0; step < INSTANT_STEP_LIMIT && !level.isFinished(); ++step) {
        if (!level.player().isMoving()) {
            const std::optional<core::Direction> direction = nextStepToExit(level);
            if (!direction) {
                return;
            }
            level.tryMove(*direction);
        }
        level.update(INSTANT_STEP_SECONDS);
    }
}

} // namespace

ScreenshotTour::ScreenshotTour(AppContext& context, ScreenStack& screens)
    : m_context(context), m_screens(screens) {
    std::error_code error;
    std::filesystem::create_directories(OUTPUT_DIRECTORY, error);
    addFrontMenuSteps();
    addProfileMenuSteps();
    addGameplaySteps();
    addLabyrinthAndFogSteps();
    addHuntedSteps();
    addFirstPersonSteps();
    addPerformanceSteps();
}

void ScreenshotTour::addStep(std::string name, std::function<void()> setup, int frames,
                             std::function<void(int, InputState&)> everyFrame) {
    m_steps.push_back(Step{std::move(name), std::move(setup), frames, std::move(everyFrame)});
}

void ScreenshotTour::createDemoProfile(core::ViewMode mode) {
    core::SaveData data;
    data.playerName = "Neo";
    data.mazeSize = 15;
    data.viewMode = mode;
    const std::array<int, 7> stars = {3, 3, 2, 3, 1, 2, 3};
    for (std::size_t index = 0; index < stars.size(); ++index) {
        const int level = static_cast<int>(index) + 1;
        data.progress.applyResult(level, stars[index], 30000 + level * 1700);
    }
    for (int tier = 0; tier < core::TIER_COUNT; ++tier) {
        data.progress.markTierIntroSeen(static_cast<core::Tier>(tier));
    }
    core::LevelSnapshot snapshot;
    snapshot.level = 8;
    snapshot.seed = DEMO_SEED;
    data.snapshot = snapshot;
    data.lastPlayed = "2026-10-18 21:40";
    m_context.profiles.createProfile(1, std::move(data));
}

void ScreenshotTour::startScriptedLevel(int level, int size, core::ViewMode mode,
                                        std::uint32_t seed) {
    m_context.pendingSession = std::make_unique<GameSession>(m_context, level, size, seed, mode);
    m_context.pendingSessionEnd = false;
    m_context.navigator.replaceWith(ScreenId::Gameplay);
}

void ScreenshotTour::walkTowardsExit() {
    if (!m_context.session) {
        return;
    }
    core::Level& level = m_context.session->level();
    if (!level.player().isMoving()) {
        if (const std::optional<core::Direction> direction = nextStepToExit(level)) {
            faceTowards(level, *direction);
            level.tryMove(*direction);
        }
    }
}

ScreenshotTour::StepAction ScreenshotTour::walker(int interval) {
    return [this, interval](int frame, InputState& /*input*/) {
        if (frame > SETTLE_FRAMES && frame % interval == 0) {
            walkTowardsExit();
        }
    };
}

ScreenshotTour::StepAction ScreenshotTour::firstPersonWalker() {
    return [this](int frame, InputState& /*input*/) {
        if (frame > SETTLE_FRAMES && frame < FIRST_PERSON_LOOK_FRAME &&
            frame % (2 * WALK_INTERVAL_FRAMES) == 0) {
            walkTowardsExit();
        }
        if (frame == FIRST_PERSON_LOOK_FRAME && m_context.session) {
            faceLongestCorridor(m_context.session->level());
        }
    };
}

void ScreenshotTour::addFrontMenuSteps() {
    addStep("01_splash", nullptr, 75);
    addStep("02_main_menu", [this] { m_context.navigator.replaceWith(ScreenId::MainMenu); }, 120);
    addStep(
        "03_new_game_size", [this] { m_context.navigator.replaceWith(ScreenId::NewGame); }, 130,
        [](int frame, InputState& input) {
            if (frame == 40 || frame == 70) {
                input.confirm = true; // pick slot 1, then confirm the typed name
            } else if (frame == 55) {
                input.typed = "Neo";
            } else if (frame >= 85 && frame < 90) {
                input.navigateRight = true; // slider 15 -> 20
            }
        });
}

void ScreenshotTour::addProfileMenuSteps() {
    const StepAction openSecondTab = [](int frame, InputState& input) {
        input.navigateRight = frame == 50;
    };
    addStep(
        "04_level_select",
        [this] {
            createDemoProfile(core::ViewMode::Classic2D);
            m_context.navigator.replaceWith(ScreenId::LevelSelect);
        },
        90);
    addStep(
        "07_load_game",
        [this] {
            core::SaveSlot corrupt;
            corrupt.status = core::SlotStatus::Corrupt;
            corrupt.error = "missing or invalid size";
            m_context.profiles.slots()[2] = corrupt;
            m_context.navigator.replaceWith(ScreenId::LoadGame);
        },
        80);
    addStep(
        "05_settings_skins", [this] { m_context.navigator.push(ScreenId::Settings); }, 100,
        openSecondTab);
    addStep(
        "08_skins_unlocked",
        [this] {
            if (core::SaveData* profile = m_context.profiles.activeProfile()) {
                profile->progress.restoreSkinMask(ALL_SKINS_MASK);
                profile->progress.selectSkin(core::SkinId::Comet);
            }
            m_context.navigator.replaceWith(ScreenId::Settings);
        },
        100, openSecondTab);
    addStep(
        "06_help_perks", [this] { m_context.navigator.replaceWith(ScreenId::Help); }, 100,
        openSecondTab);
}

void ScreenshotTour::addGameplaySteps() {
    auto& gameplay = static_cast<ui::GameplayScreen&>(m_screens.screen(ScreenId::Gameplay));
    addStep(
        "10_gameplay_tier1",
        [this] { startScriptedLevel(1, 15, core::ViewMode::Classic2D, DEMO_SEED); }, 170,
        walker(WALK_INTERVAL_FRAMES));
    addStep("11_text_view", [this] { m_context.session->setTextViewVisible(true); }, 30);
    addStep(
        "12_pause_menu",
        [this, &gameplay] {
            m_context.session->setTextViewVisible(false);
            gameplay.openPause();
        },
        30);
    addStep(
        "13_level_complete",
        [this, &gameplay] {
            gameplay.closePause();
            finishLevelInstantly(m_context.session->level());
        },
        200);
}

void ScreenshotTour::addLabyrinthAndFogSteps() {
    const StepAction walk = walker(WALK_INTERVAL_FRAMES);
    addStep(
        "20_tier2_labyrinth",
        [this] { startScriptedLevel(7, 18, core::ViewMode::Classic2D, DEMO_SEED + 7U); }, 150,
        walk);
    addStep(
        "25_perks_phase_compass",
        [this] { startScriptedLevel(9, 16, core::ViewMode::Classic2D, DEMO_SEED + 9U); }, 75,
        [this](int frame, InputState& /*input*/) {
            if (frame == SETTLE_FRAMES && m_context.session) {
                showcasePerks(m_context.session->level());
            }
        });
    addStep(
        "21_tier3_intro_card",
        [this] {
            if (core::SaveData* profile = m_context.profiles.activeProfile()) {
                profile->progress.restoreTierIntroMask(FOG_UNSEEN_INTRO_MASK);
            }
            startScriptedLevel(12, 20, core::ViewMode::Classic2D, DEMO_SEED + 12U);
        },
        80);
    addStep("22_tier3_fog", nullptr, 190, [walk](int frame, InputState& input) {
        input.confirm = frame == 2; // dismiss the intro card
        walk(frame, input);
    });
}

void ScreenshotTour::addHuntedSteps() {
    addStep(
        "23_tier4_hunted",
        [this] { startScriptedLevel(17, 30, core::ViewMode::Classic2D, DEMO_SEED + 17U); }, 80,
        [this](int frame, InputState& /*input*/) {
            if (frame == SETTLE_FRAMES && m_context.session) {
                core::Level& level = m_context.session->level();
                walkInstantly(level, 6);
                simulate(level, core::CHASER_GRACE_SECONDS + 2.0f);
            }
        });
    addStep("24_caught", nullptr, 130, [this](int frame, InputState& /*input*/) {
        if (frame == 1 && m_context.session) {
            simulate(m_context.session->level(), SIMULATION_LIMIT_SECONDS);
        }
    });
}

void ScreenshotTour::addFirstPersonSteps() {
    addStep(
        "30_3d_tier1",
        [this] { startScriptedLevel(2, 15, core::ViewMode::FirstPerson3D, DEMO_SEED + 2U); }, 150,
        firstPersonWalker());
    addStep("31_3d_minimap_large", [this] { m_context.session->toggleMinimapSize(); }, 40);
    addStep(
        "32_3d_tier3_fog",
        [this] { startScriptedLevel(13, 20, core::ViewMode::FirstPerson3D, DEMO_SEED + 13U); }, 150,
        firstPersonWalker());
    addStep(
        "33_3d_tier4_hunted",
        [this] { startScriptedLevel(18, 16, core::ViewMode::FirstPerson3D, DEMO_SEED + 18U); }, 90,
        [this](int frame, InputState& /*input*/) {
            if (frame == SETTLE_FRAMES && m_context.session) {
                core::Level& level = m_context.session->level();
                walkInstantly(level, 4);
                simulate(level, core::CHASER_GRACE_SECONDS + 2.5f);
                faceLongestCorridor(level);
            }
        });
}

void ScreenshotTour::addPerformanceSteps() {
    // Largest mazes with the FPS counter on. The tour never saves settings, so switching the
    // counter on here does not leak into the real settings file.
    addStep(
        "34_perf_3d_50x50",
        [this] {
            m_context.profiles.settings().showFps = true;
            startScriptedLevel(4, core::MAX_MAZE_SIZE, core::ViewMode::FirstPerson3D, DEMO_SEED);
        },
        180, firstPersonWalker());
    addStep(
        "35_perf_2d_50x50",
        [this] {
            startScriptedLevel(4, core::MAX_MAZE_SIZE, core::ViewMode::Classic2D, DEMO_SEED);
        },
        150, walker(WALK_INTERVAL_FRAMES));
}

void ScreenshotTour::showcasePerks(core::Level& level) {
    // Walk a few steps, then grant berries and effects so one capture shows them all.
    walkInstantly(level, 3);
    core::LevelSnapshot boosted = level.snapshot();
    boosted.berries = 2;
    boosted.effectMs[static_cast<std::size_t>(core::EffectType::CompassTrail)] = 2800;
    boosted.effectMs[static_cast<std::size_t>(core::EffectType::TimeFreeze)] = 3500;
    level.restore(boosted);
    level.activateBerry();
}

InputState ScreenshotTour::beforeUpdate(const InputState& realInput) {
    InputState input = emptyInput(realInput);
    if (finished()) {
        return input;
    }
    Step& step = m_steps[m_index];
    if (m_frame == 0 && step.setup) {
        step.setup();
    }
    if (step.everyFrame) {
        step.everyFrame(m_frame, input);
    }
    ++m_frame;
    return input;
}

void ScreenshotTour::afterDraw() {
    if (finished() || m_frame < m_steps[m_index].frames) {
        return;
    }
    const std::string path = std::string(OUTPUT_DIRECTORY) + "/" + m_steps[m_index].name + ".png";
    rlDrawRenderBatchActive(); // flush pending batched geometry before reading pixels
    Image image = LoadImageFromScreen();
    ExportImage(image, path.c_str());
    UnloadImage(image);
    ++m_index;
    m_frame = 0;
}

} // namespace app
