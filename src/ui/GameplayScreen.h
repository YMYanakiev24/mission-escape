#pragma once

#include "app/AppContext.h"
#include "ui/ConfirmDialog.h"
#include "ui/Screen.h"
#include "ui/Widgets.h"

/// @file GameplayScreen.h
/// The in-game screen: runs the game session and hosts the pause menu.

namespace ui {

/// Plays the current level; Esc / P (or losing window focus) opens the pause menu.
class GameplayScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit GameplayScreen(app::AppContext& context);

    /// Resets the screen each time it becomes active.
    void onEnter() override;
    /// Called when a screen pushed on top of this one is closed.
    void onResume() override;
    /// Handles input and advances the screen by one frame.
    void update(float deltaSeconds, const app::InputState& input) override;
    /// Draws the screen.
    void draw() override;

    /// Opens the pause menu (also used by the screenshot tour).
    void openPause();

    /// Closes the pause menu and resumes play.
    void closePause() noexcept { m_paused = false; }

private:
    void updatePause(const app::InputState& input);
    void handlePauseAction(const Widget* fired);
    void handleConfirmDialog(const app::InputState& input);
    void layoutPause();
    void drawPause() const;
    void checkFinished();
    void updateTierIntro(const app::InputState& input);
    void drawTierIntro() const;

    app::AppContext& m_context;
    bool m_paused = false;
    bool m_resultShown = false;
    bool m_showTierIntro = false; ///< First visit to this tier: explain what changed.
    Button m_resume{"Resume", ButtonStyle::Primary};
    Button m_restart{"Restart Level"};
    Button m_settings{"Settings"};
    Button m_help{"Help"};
    Button m_saveAndExit{"Save & Exit to Menu"};
    Button m_quitLevel{"Quit Level"};
    FocusGroup m_pauseFocus;
    ConfirmDialog m_confirm;
};

} // namespace ui
