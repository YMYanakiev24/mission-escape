#pragma once

#include <raylib.h>

#include <string>

/// @file Input.h
/// Translates raw raylib keyboard, mouse and gamepad state into menu input and gameplay actions.

namespace app {

/// Snapshot of the user's input for one frame, consumed by menus and widgets.
struct InputState {
    Vector2 mouse{};
    bool mouseMoved = false;
    bool mousePressed = false;  ///< Left button went down this frame.
    bool mouseDown = false;     ///< Left button is held.
    bool mouseReleased = false; ///< Left button went up this frame.
    float wheel = 0.0f;

    bool navigateUp = false; ///< Arrow keys, D-pad or left stick, including auto-repeat.
    bool navigateDown = false;
    bool navigateLeft = false;
    bool navigateRight = false;
    bool confirm = false;   ///< Enter or gamepad A.
    bool back = false;      ///< Escape or gamepad B.
    bool backspace = false; ///< Backspace, including auto-repeat.
    bool anyPressed = false;

    std::string typed; ///< Printable ASCII characters typed this frame.
};

/// Reads the current frame's input from raylib. Call exactly once per frame: it also samples
/// the gamepad that isActionPressed() and isActionDown() read.
InputState pollInput();

/// An input state with nothing pressed (used while screens fade).
InputState emptyInput(const InputState& current);

/// Gameplay commands, independent of the physical keys that trigger them. Gamepad buttons use
/// the Xbox names (A B X Y); on PlayStation pads they are Cross, Circle, Square and Triangle.
enum class GameAction {
    Up,             ///< 2D: move north. 3D: move forward. D-pad / left stick up.
    Down,           ///< 2D: move south. 3D: move back.
    Left,           ///< 2D: move west.  3D: turn left.
    Right,          ///< 2D: move east.  3D: turn right.
    StrafeLeft,     ///< 3D only (Q, LB).
    StrafeRight,    ///< 3D only (E, RB).
    UseBerry,       ///< Shift, X or RT: activate a Magic Berry.
    Pause,          ///< Esc, P or Start.
    Restart,        ///< R or Select: start the level over on the same maze.
    ToggleTextView, ///< T or Y.
    ToggleMinimap   ///< M or LT.
};

/// True on the frame any key or gamepad button bound to @p action is pressed.
bool isActionPressed(GameAction action);

/// True while any key or gamepad button bound to @p action is held down.
bool isActionDown(GameAction action);

/// True when the gamepad was used more recently than the keyboard or mouse, so on-screen
/// hints can name gamepad buttons instead of keys.
bool isUsingGamepad();

} // namespace app
