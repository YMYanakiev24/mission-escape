#include "app/Input.h"

#include <array>
#include <cmath>

namespace app {

namespace {

constexpr int FIRST_PRINTABLE = 32;
constexpr int LAST_PRINTABLE = 126;
constexpr float MOUSE_MOVE_EPSILON = 0.01f;

/// Up to three keys can trigger one action; KEY_NULL marks unused entries.
using KeyBinding = std::array<KeyboardKey, 3>;

KeyBinding bindingFor(GameAction action) {
    switch (action) {
        case GameAction::Up:
            return {KEY_UP, KEY_W, KEY_NULL};
        case GameAction::Down:
            return {KEY_DOWN, KEY_S, KEY_NULL};
        case GameAction::Left:
            return {KEY_LEFT, KEY_A, KEY_NULL};
        case GameAction::Right:
            return {KEY_RIGHT, KEY_D, KEY_NULL};
        case GameAction::StrafeLeft:
            return {KEY_Q, KEY_NULL, KEY_NULL};
        case GameAction::StrafeRight:
            return {KEY_E, KEY_NULL, KEY_NULL};
        case GameAction::UseBerry:
            return {KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT, KEY_NULL};
        case GameAction::Pause:
            return {KEY_ESCAPE, KEY_P, KEY_NULL};
        case GameAction::ToggleTextView:
            return {KEY_T, KEY_NULL, KEY_NULL};
        case GameAction::ToggleMinimap:
            return {KEY_M, KEY_NULL, KEY_NULL};
    }
    return {KEY_NULL, KEY_NULL, KEY_NULL};
}

bool pressedOrRepeated(KeyboardKey key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

void readTypedCharacters(InputState& state) {
    for (int codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed()) {
        if (codepoint >= FIRST_PRINTABLE && codepoint <= LAST_PRINTABLE) {
            state.typed.push_back(static_cast<char>(codepoint));
        }
    }
}

} // namespace

InputState pollInput() {
    InputState state;
    state.mouse = GetMousePosition();
    const Vector2 delta = GetMouseDelta();
    state.mouseMoved =
        std::fabs(delta.x) > MOUSE_MOVE_EPSILON || std::fabs(delta.y) > MOUSE_MOVE_EPSILON;
    state.mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    state.mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    state.mouseReleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    state.wheel = GetMouseWheelMove();
    state.navigateUp = pressedOrRepeated(KEY_UP);
    state.navigateDown = pressedOrRepeated(KEY_DOWN);
    state.navigateLeft = pressedOrRepeated(KEY_LEFT);
    state.navigateRight = pressedOrRepeated(KEY_RIGHT);
    state.confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    state.back = IsKeyPressed(KEY_ESCAPE);
    state.backspace = pressedOrRepeated(KEY_BACKSPACE);
    state.anyPressed = GetKeyPressed() != 0 || state.mousePressed;
    readTypedCharacters(state);
    return state;
}

InputState emptyInput(const InputState& current) {
    InputState state;
    state.mouse = current.mouse;
    return state;
}

bool isActionPressed(GameAction action) {
    for (const KeyboardKey key : bindingFor(action)) {
        if (key != KEY_NULL && IsKeyPressed(key)) {
            return true;
        }
    }
    return false;
}

bool isActionDown(GameAction action) {
    for (const KeyboardKey key : bindingFor(action)) {
        if (key != KEY_NULL && IsKeyDown(key)) {
            return true;
        }
    }
    return false;
}

} // namespace app
