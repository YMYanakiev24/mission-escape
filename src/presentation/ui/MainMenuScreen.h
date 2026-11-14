#pragma once

#include "app/AppContext.h"
#include "ui/Screen.h"
#include "ui/Widgets.h"

#include <string>

/// @file MainMenuScreen.h
/// The main menu: Continue, New Game, Load Game, Settings, Help, Quit.

namespace ui {

/// Entry point of the game, drawn over the self-generating maze backdrop.
class MainMenuScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit MainMenuScreen(app::AppContext& context);

    /// Resets the screen each time it becomes active.
    void onEnter() override;
    /// Handles input and advances the screen by one frame.
    void update(float deltaSeconds, const app::InputState& input) override;
    /// Draws the screen.
    void draw() override;

private:
    void layout();
    void handleAction(const Widget* fired);
    void continueGame();
    bool canContinue() const;

    app::AppContext& m_context;
    Button m_continue{"Continue", ButtonStyle::Primary};
    Button m_newGame{"New Game"};
    Button m_loadGame{"Load Game"};
    Button m_settings{"Settings"};
    Button m_help{"Help"};
    Button m_quit{"Quit"};
    FocusGroup m_focus;
    std::string m_notice; ///< Save or resume problem to show briefly.
    float m_noticeSeconds = 0.0f;
};

} // namespace ui
