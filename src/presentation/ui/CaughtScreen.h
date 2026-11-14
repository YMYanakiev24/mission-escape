#pragma once

#include "app/AppContext.h"
#include "ui/Screen.h"
#include "ui/Widgets.h"

/// @file CaughtScreen.h
/// Overlay shown when the tier 4 chaser catches the player.

namespace ui {

/// "Caught!" result with Retry and Level Select. A caught attempt scores 0 stars.
class CaughtScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit CaughtScreen(app::AppContext& context);

    /// Resets the screen each time it becomes active.
    void onEnter() override;
    /// Handles input and advances the screen by one frame.
    void update(float deltaSeconds, const app::InputState& input) override;
    /// Draws the screen.
    void draw() override;
    /// Result overlays are drawn on top of the gameplay screen.
    bool isOverlay() const override { return true; }

private:
    void layout();
    Rectangle panelBounds() const;

    app::AppContext& m_context;
    float m_elapsed = 0.0f;
    Button m_retry{"Retry", ButtonStyle::Primary};
    Button m_levelSelect{"Level Select"};
    FocusGroup m_focus;
};

} // namespace ui
