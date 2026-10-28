#pragma once

#include "app/AppContext.h"
#include "ui/Screen.h"

/// @file SplashScreen.h
/// Short animated title card shown at start-up.

namespace ui {

/// Fades the game title in; continues to the main menu on any key or after a few seconds.
class SplashScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit SplashScreen(app::AppContext& context) : m_context(context) {}

    /// Resets the screen each time it becomes active.
    void onEnter() override { m_elapsed = 0.0f; }
    /// Handles input and advances the screen by one frame.
    void update(float deltaSeconds, const app::InputState& input) override;
    /// Draws the screen.
    void draw() override;

private:
    app::AppContext& m_context;
    float m_elapsed = 0.0f;
};

} // namespace ui
