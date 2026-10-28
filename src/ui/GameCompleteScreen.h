#pragma once

#include "app/AppContext.h"
#include "ui/Screen.h"
#include "ui/Widgets.h"

/// @file GameCompleteScreen.h
/// Celebration shown after the final level.

namespace ui {

/// Congratulates the player after level 20 and shows the campaign totals.
class GameCompleteScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit GameCompleteScreen(app::AppContext& context);

    /// Resets the screen each time it becomes active.
    void onEnter() override;
    /// Handles input and advances the screen by one frame.
    void update(float deltaSeconds, const app::InputState& input) override;
    /// Draws the screen.
    void draw() override;

private:
    void layout();

    app::AppContext& m_context;
    float m_elapsed = 0.0f;
    Button m_levelSelect{"Level Select", ButtonStyle::Primary};
    Button m_mainMenu{"Main Menu"};
    FocusGroup m_focus;
};

} // namespace ui
