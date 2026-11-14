#pragma once

#include "app/AppContext.h"
#include "app/Input.h"
#include "app/ScreenStack.h"
#include "core/Level.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

/// @file ScreenshotTour.h
/// `--screenshots` mode: walks through the screens automatically and saves PNG captures.

namespace app {

/// Scripted walk through the game that captures one PNG per step into `screenshots/`.
///
/// Each step runs a setup action (navigate, start a level, open a menu...), lets the game run
/// for a number of frames so fades and animations settle, may inject synthetic input or act
/// on the game every frame (e.g. walking the player), and finally captures the frame.
/// The tour works on an in-memory profile; nothing is written to the real save slots.
class ScreenshotTour {
public:
    /// Prepares every step and the output folder.
    ScreenshotTour(AppContext& context, ScreenStack& screens);

    /// Runs the current step's actions and returns the (possibly scripted) input to use.
    InputState beforeUpdate(const InputState& realInput);

    /// Captures the frame if the current step is due; call after drawing, before EndDrawing().
    void afterDraw();

    /// True when all steps have been captured.
    bool finished() const noexcept { return m_index >= m_steps.size(); }

private:
    /// Called every frame of a step with the frame number; may modify that frame's input.
    using StepAction = std::function<void(int, InputState&)>;

    /// One scripted capture.
    struct Step {
        std::string name;
        std::function<void()> setup;
        int frames = 60;
        StepAction everyFrame;
    };

    void addStep(std::string name, std::function<void()> setup, int frames,
                 StepAction everyFrame = nullptr);
    void addFrontMenuSteps();
    void addProfileMenuSteps();
    void addGameplaySteps();
    void addLabyrinthAndFogSteps();
    void addHuntedSteps();
    void addFirstPersonSteps();
    void addPerformanceSteps();
    StepAction walker(int interval);
    StepAction firstPersonWalker();
    void showcasePerks(core::Level& level);
    void createDemoProfile(core::ViewMode mode);
    void walkTowardsExit();
    void startScriptedLevel(int level, int size, core::ViewMode mode, std::uint32_t seed);

    AppContext& m_context;
    ScreenStack& m_screens;
    std::vector<Step> m_steps;
    std::size_t m_index = 0;
    int m_frame = 0;
};

} // namespace app
