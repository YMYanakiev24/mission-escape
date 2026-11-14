#pragma once

#include <optional>

/// @file Navigator.h
/// Screen identifiers and the request queue screens use to change the active screen.

namespace app {

/// Every state of the top level screen state machine.
enum class ScreenId {
    Splash,
    MainMenu,
    NewGame,
    LoadGame,
    LevelSelect,
    Settings,
    Help,
    Gameplay,
    LevelComplete,
    Caught,
    GameComplete,
    Count
};

/// Number of real screens (excluding the Count sentinel).
inline constexpr int SCREEN_COUNT = static_cast<int>(ScreenId::Count);

/// How the screen stack should change.
enum class NavigationKind {
    Replace, ///< Clear the stack and show the target.
    Push,    ///< Show the target on top; the previous screen resumes when it is popped.
    Pop,     ///< Return to the screen below.
    Quit     ///< Close the application.
};

/// One requested stack change.
struct NavigationRequest {
    NavigationKind kind = NavigationKind::Replace;
    ScreenId target = ScreenId::MainMenu;
};

/// Collects the navigation request of the current frame; the App applies it with a fade.
class Navigator {
public:
    /// Clears the stack and shows @p target.
    void replaceWith(ScreenId target) { request(NavigationKind::Replace, target); }

    /// Shows @p target on top of the current screen.
    void push(ScreenId target) { request(NavigationKind::Push, target); }

    /// Returns to the previous screen.
    void pop() { request(NavigationKind::Pop, ScreenId::MainMenu); }

    /// Asks the application to close.
    void quit() { request(NavigationKind::Quit, ScreenId::MainMenu); }

    /// True when a request is waiting to be applied.
    bool hasPending() const noexcept { return m_pending.has_value(); }

    /// Removes and returns the pending request.
    std::optional<NavigationRequest> take() {
        std::optional<NavigationRequest> request = m_pending;
        m_pending.reset();
        return request;
    }

private:
    void request(NavigationKind kind, ScreenId target) {
        // The first request of a frame wins; later ones (e.g. from a second button) are ignored.
        if (!m_pending) {
            m_pending = NavigationRequest{kind, target};
        }
    }

    std::optional<NavigationRequest> m_pending;
};

} // namespace app
