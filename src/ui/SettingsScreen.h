#pragma once

#include "app/AppContext.h"
#include "core/Progress.h"
#include "ui/Screen.h"
#include "ui/TabBar.h"
#include "ui/Widgets.h"

#include <array>

/// @file SettingsScreen.h
/// Audio volumes, skin selection and display options.

namespace ui {

/// Animated preview card for one skin; locked cards explain how to unlock them.
class SkinCard : public Widget {
public:
    /// Sets which skin the card shows and its state.
    void configure(core::SkinId skin, bool unlocked, bool selected);

    /// Skin shown on the card.
    core::SkinId skin() const noexcept { return m_skin; }

    /// Reacts to this frame's input; returns true when the widget fired.
    bool handleInput(const app::InputState& input, bool focused) override;
    /// Draws the widget (highlighted when focused).
    void draw(const render::Theme& theme, bool focused, float time) const override;

private:
    core::SkinId m_skin = core::SkinId::Wisp;
    bool m_unlocked = false;
    bool m_selected = false;
};

/// Tabbed settings: Audio, Skins, Display. Changes apply immediately and are saved on exit.
class SettingsScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit SettingsScreen(app::AppContext& context);

    /// Resets the screen each time it becomes active.
    void onEnter() override;
    /// Handles input and advances the screen by one frame.
    void update(float deltaSeconds, const app::InputState& input) override;
    /// Draws the screen.
    void draw() override;

private:
    void rebuildFocus();
    void layout();
    void loadValues();
    void applyChanges(const Widget* fired);
    void refreshSkinCards();
    void leave();

    app::AppContext& m_context;
    TabBar<3> m_tabs{{"Audio", "Skins", "Display"}};
    std::size_t m_builtTab = 99;
    Slider m_master{"Master volume", 0, 100, 5};
    Slider m_music{"Music volume", 0, 100, 5};
    Slider m_sfx{"Effects volume", 0, 100, 5};
    Toggle m_mute{"Mute all sound"};
    std::array<SkinCard, core::SKIN_COUNT> m_skinCards;
    Toggle m_fullscreen{"Fullscreen (borderless)"};
    Toggle m_showFps{"Show FPS counter"};
    Toggle m_headBob{"Head bob in 3D mode"};
    Button m_back{"Back"};
    FocusGroup m_focus;
};

} // namespace ui
