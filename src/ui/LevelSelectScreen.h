#pragma once

#include "app/AppContext.h"
#include "core/Config.h"
#include "ui/Screen.h"
#include "ui/Widgets.h"

#include <raylib.h>

#include <array>

/// @file LevelSelectScreen.h
/// Grid of the 20 levels grouped by tier, with best stars and locks.

namespace ui {

/// One level button: number, best stars, lock icon when not yet unlocked.
class LevelTile : public Widget {
public:
    /// Updates what the tile shows; locked tiles are disabled (not focusable).
    void configure(int level, bool unlocked, int bestStars, Color accent);

    /// Level number shown on the tile.
    int level() const noexcept { return m_level; }

    /// Reacts to this frame's input; returns true when the widget fired.
    bool handleInput(const app::InputState& input, bool focused) override;
    /// Draws the widget (highlighted when focused).
    void draw(const render::Theme& theme, bool focused, float time) const override;

private:
    int m_level = 1;
    int m_bestStars = 0;
    Color m_accent{};
};

/// Lets the player pick an unlocked level, switch the view mode or go back.
class LevelSelectScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit LevelSelectScreen(app::AppContext& context);

    /// Resets the screen each time it becomes active.
    void onEnter() override;
    /// Handles input and advances the screen by one frame.
    void update(float deltaSeconds, const app::InputState& input) override;
    /// Draws the screen.
    void draw() override;

private:
    void layout();
    void refreshTiles();
    void handleAction(const Widget* fired);
    void drawTierLabels() const;

    app::AppContext& m_context;
    std::array<LevelTile, core::LEVEL_COUNT> m_tiles;
    Button m_viewMode{"View"};
    Button m_back{"Back"};
    FocusGroup m_focus;
};

} // namespace ui
