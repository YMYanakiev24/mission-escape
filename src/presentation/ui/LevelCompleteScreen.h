#pragma once

#include "app/AppContext.h"
#include "ui/Screen.h"
#include "ui/Widgets.h"

/// @file LevelCompleteScreen.h
/// Result overlay shown after the exit is reached.

namespace ui {

/// Shows the time, an animated star reveal and the personal best, then offers
/// Next / Retry / Level Select.
class LevelCompleteScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit LevelCompleteScreen(app::AppContext& context);

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
    void drawStars(Rectangle panel) const;
    void playStarDings();
    void drawDetails(Rectangle panel) const;

    app::AppContext& m_context;
    float m_elapsed = 0.0f;
    int m_starsAnnounced = 0;
    Button m_next{"Next Level", ButtonStyle::Primary};
    Button m_retry{"Retry"};
    Button m_levelSelect{"Level Select"};
    FocusGroup m_focus;
};

} // namespace ui
