#pragma once

#include "app/AppContext.h"
#include "ui/Screen.h"
#include "ui/TabBar.h"
#include "ui/Widgets.h"

/// @file HelpScreen.h
/// Controls, perk legend, tier rules and star rules.

namespace ui {

/// Tabbed help pages; reachable from the main menu and the pause menu.
class HelpScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit HelpScreen(app::AppContext& context);

    /// Resets the screen each time it becomes active.
    void onEnter() override;
    /// Handles input and advances the screen by one frame.
    void update(float deltaSeconds, const app::InputState& input) override;
    /// Draws the screen.
    void draw() override;

private:
    void layout();
    void drawControls(Rectangle area) const;
    void drawPerks(Rectangle area) const;
    void drawTiers(Rectangle area) const;
    void drawStars(Rectangle area) const;

    app::AppContext& m_context;
    TabBar<4> m_tabs{{"Controls", "Perks", "Tiers", "Stars"}};
    Button m_back{"Back"};
    FocusGroup m_focus;
};

} // namespace ui
