#pragma once

#include <raylib.h>

#include <string>

/// @file Input.h
/// Translates raw raylib keyboard and mouse state into menu input and gameplay actions.

namespace app {

/// Snapshot of the user's input for one frame, consumed by menus and widgets.
struct InputState {
    Vector2 mouse{};
    bool mouseMoved = false;
    bool mousePressed = false;  ///< Left button went down this frame.
    bool mouseDown = false;     ///< Left button is held.
    bool mouseReleased = false; ///< Left button went up this frame.
    float wheel = 0.0f;

    bool navigateUp = false; ///< Arrow keys, including keyboard auto-repeat.
    bool navigateDown = false;
    bool navigateLeft = false;
    bool navigateRight = false;
    bool confirm = false;   ///< Enter.
    bool back = false;      ///< Escape.
    bool backspace = false; ///< Backspace, including auto-repeat.
    bool anyPressed = false;

    std::string typed; ///< Printable ASCII characters typed this frame.
};

/// Reads the current frame's input from raylib.
InputState pollInput();

/// An input state with nothing pressed (used while screens fade).
InputState emptyInput(const InputState& current);

/// Gameplay commands, independent of the physical keys that trigger them.
enum class GameAction {
    Up,             ///< 2D: move north. 3D: move forward.
    Down,           ///< 2D: move south. 3D: move back.
    Left,           ///< 2D: move west.  3D: turn left.
    Right,          ///< 2D: move east.  3D: turn right.
    StrafeLeft,     ///< 3D only (Q).
    StrafeRight,    ///< 3D only (E).
    UseBerry,       ///< Shift: activate a Magic Berry.
    Pause,          ///< Esc or P.
    ToggleTextView, ///< T.
    ToggleMinimap   ///< M.
};

/// True on the frame any key bound to @p action is pressed.
bool isActionPressed(GameAction action);

/// True while any key bound to @p action is held down.
bool isActionDown(GameAction action);

} // namespace app
