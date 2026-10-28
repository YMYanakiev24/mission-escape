#pragma once

#include "app/AppContext.h"
#include "app/Input.h"
#include "app/Navigator.h"
#include "ui/Screen.h"

#include <array>
#include <memory>
#include <optional>
#include <vector>

/// @file ScreenStack.h
/// The screen state machine: a stack of screens with fade transitions between states.

namespace app {

/// Owns one instance of every screen and the stack of active screens.
///
/// Navigation requests are never applied immediately: the stack fades to black, switches
/// screens (and applies pending session changes) on the black frame, then fades back in.
class ScreenStack {
public:
    /// Creates all screens and shows @p initial.
    ScreenStack(AppContext& context, ScreenId initial);

    /// Updates the top screen and advances transitions.
    void update(float deltaSeconds, const InputState& input);

    /// Draws the visible screens and the fade overlay.
    void draw();

    /// True once a screen asked the application to quit.
    bool quitRequested() const noexcept { return m_quit; }

    /// The screen currently on top.
    ScreenId top() const { return m_stack.back(); }

    /// True while a fade is running.
    bool isTransitioning() const noexcept { return m_phase != Phase::Idle; }

    /// The single instance of screen @p id.
    ui::Screen& screen(ScreenId id);

private:
    enum class Phase { Idle, FadingOut, FadingIn };

    void startTransition();
    void applyRequest(const NavigationRequest& request);

    AppContext& m_context;
    std::array<std::unique_ptr<ui::Screen>, SCREEN_COUNT> m_screens;
    std::vector<ScreenId> m_stack;
    std::optional<NavigationRequest> m_request;
    Phase m_phase = Phase::FadingIn;
    float m_fade = 1.0f; ///< 0 = fully visible, 1 = black.
    bool m_quit = false;
};

} // namespace app
