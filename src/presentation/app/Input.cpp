#include "app/Input.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace app {

namespace {

constexpr int FIRST_PRINTABLE = 32;
constexpr int LAST_PRINTABLE = 126;
constexpr float MOUSE_MOVE_EPSILON = 0.01f;

constexpr int MAX_GAMEPADS = 4;
/// The left stick must be pushed this far (0..1) before it counts as a direction.
constexpr float STICK_DEADZONE = 0.5f;
/// Menu auto-repeat for a held D-pad or stick, matching the feel of keyboard repeat.
constexpr float PAD_REPEAT_DELAY = 0.40f;
constexpr float PAD_REPEAT_INTERVAL = 0.11f;

/// Up to three keys can trigger one action; KEY_NULL marks unused entries.
using KeyBinding = std::array<KeyboardKey, 3>;

/// Up to two gamepad buttons can trigger one action; GAMEPAD_BUTTON_UNKNOWN marks unused ones.
using PadBinding = std::array<GamepadButton, 2>;

/// The four directions of the D-pad and the left stick, in this order.
enum PadDirection : std::size_t { PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_DIRECTION_COUNT };

constexpr std::array<GamepadButton, PAD_DIRECTION_COUNT> DPAD_BUTTONS = {
    GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_DOWN, GAMEPAD_BUTTON_LEFT_FACE_LEFT,
    GAMEPAD_BUTTON_LEFT_FACE_RIGHT};

/// Gamepad state sampled once per frame by pollInput(), so every query in a frame agrees and
/// the stick gets the same "pressed this frame" edges as a button.
struct GamepadFrame {
    int pad = -1; ///< First connected gamepad, or -1 when none is connected.
    std::array<bool, PAD_DIRECTION_COUNT> held{};
    std::array<bool, PAD_DIRECTION_COUNT> pressed{};  ///< Held now but not last frame.
    std::array<bool, PAD_DIRECTION_COUNT> repeated{}; ///< Pressed, or auto-repeated for menus.
    std::array<float, PAD_DIRECTION_COUNT> nextRepeat{};
    std::array<float, PAD_DIRECTION_COUNT> heldSeconds{};
    bool usedLast = false; ///< The gamepad was touched after the keyboard or mouse.
};

GamepadFrame& gamepadFrame() {
    static GamepadFrame frame;
    return frame;
}

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
        case GameAction::Restart:
            return {KEY_R, KEY_NULL, KEY_NULL};
        case GameAction::ToggleTextView:
            return {KEY_T, KEY_NULL, KEY_NULL};
        case GameAction::ToggleMinimap:
            return {KEY_M, KEY_NULL, KEY_NULL};
    }
    return {KEY_NULL, KEY_NULL, KEY_NULL};
}

/// Buttons bound to @p action; the four movement actions use the D-pad and stick instead.
PadBinding padBindingFor(GameAction action) {
    switch (action) {
        case GameAction::StrafeLeft:
            return {GAMEPAD_BUTTON_LEFT_TRIGGER_1, GAMEPAD_BUTTON_UNKNOWN};
        case GameAction::StrafeRight:
            return {GAMEPAD_BUTTON_RIGHT_TRIGGER_1, GAMEPAD_BUTTON_UNKNOWN};
        case GameAction::UseBerry:
            return {GAMEPAD_BUTTON_RIGHT_FACE_LEFT, GAMEPAD_BUTTON_RIGHT_TRIGGER_2};
        case GameAction::Pause:
            return {GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_UNKNOWN};
        case GameAction::Restart:
            return {GAMEPAD_BUTTON_MIDDLE_LEFT, GAMEPAD_BUTTON_UNKNOWN};
        case GameAction::ToggleTextView:
            return {GAMEPAD_BUTTON_RIGHT_FACE_UP, GAMEPAD_BUTTON_UNKNOWN};
        case GameAction::ToggleMinimap:
            return {GAMEPAD_BUTTON_LEFT_TRIGGER_2, GAMEPAD_BUTTON_UNKNOWN};
        case GameAction::Up:
        case GameAction::Down:
        case GameAction::Left:
        case GameAction::Right:
            break;
    }
    return {GAMEPAD_BUTTON_UNKNOWN, GAMEPAD_BUTTON_UNKNOWN};
}

/// The pad direction behind a movement action, or PAD_DIRECTION_COUNT for other actions.
std::size_t padDirectionFor(GameAction action) {
    switch (action) {
        case GameAction::Up:
            return PAD_UP;
        case GameAction::Down:
            return PAD_DOWN;
        case GameAction::Left:
            return PAD_LEFT;
        case GameAction::Right:
            return PAD_RIGHT;
        default:
            return PAD_DIRECTION_COUNT;
    }
}

int firstConnectedGamepad() {
    for (int pad = 0; pad < MAX_GAMEPADS; ++pad) {
        if (IsGamepadAvailable(pad)) {
            return pad;
        }
    }
    return -1;
}

/// True when the D-pad or the left stick points in @p direction. Only the stronger stick
/// axis counts, so a diagonal push never means two directions at once.
bool isDirectionHeld(int pad, std::size_t direction) {
    if (IsGamepadButtonDown(pad, DPAD_BUTTONS[direction])) {
        return true;
    }
    const float x = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_X);
    const float y = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_Y);
    const bool horizontal = std::fabs(x) >= std::fabs(y);
    switch (direction) {
        case PAD_UP:
            return !horizontal && y < -STICK_DEADZONE;
        case PAD_DOWN:
            return !horizontal && y > STICK_DEADZONE;
        case PAD_LEFT:
            return horizontal && x < -STICK_DEADZONE;
        default:
            return horizontal && x > STICK_DEADZONE;
    }
}

bool anyGamepadButtonPressed(int pad) {
    for (int button = GAMEPAD_BUTTON_LEFT_FACE_UP; button <= GAMEPAD_BUTTON_RIGHT_THUMB; ++button) {
        if (IsGamepadButtonPressed(pad, button)) {
            return true;
        }
    }
    return false;
}

/// Updates the per-frame gamepad state; returns true if the gamepad was used this frame.
bool sampleGamepad(GamepadFrame& frame, float deltaSeconds) {
    frame.pad = firstConnectedGamepad();
    bool used = false;
    for (std::size_t direction = 0; direction < PAD_DIRECTION_COUNT; ++direction) {
        const bool held = frame.pad >= 0 && isDirectionHeld(frame.pad, direction);
        frame.pressed[direction] = held && !frame.held[direction];
        frame.repeated[direction] = frame.pressed[direction];
        if (frame.pressed[direction]) {
            frame.heldSeconds[direction] = 0.0f;
            frame.nextRepeat[direction] = PAD_REPEAT_DELAY;
        } else if (held) {
            frame.heldSeconds[direction] += deltaSeconds;
            if (frame.heldSeconds[direction] >= frame.nextRepeat[direction]) {
                frame.repeated[direction] = true;
                frame.nextRepeat[direction] += PAD_REPEAT_INTERVAL;
            }
        }
        frame.held[direction] = held;
        used = used || frame.pressed[direction];
    }
    return used || (frame.pad >= 0 && anyGamepadButtonPressed(frame.pad));
}

bool isPadButtonPressed(GamepadButton button) {
    const int pad = gamepadFrame().pad;
    return pad >= 0 && IsGamepadButtonPressed(pad, button);
}

bool isPadActionPressed(GameAction action) {
    const GamepadFrame& frame = gamepadFrame();
    if (frame.pad < 0) {
        return false;
    }
    const std::size_t direction = padDirectionFor(action);
    if (direction != PAD_DIRECTION_COUNT) {
        return frame.pressed[direction];
    }
    for (const GamepadButton button : padBindingFor(action)) {
        if (button != GAMEPAD_BUTTON_UNKNOWN && IsGamepadButtonPressed(frame.pad, button)) {
            return true;
        }
    }
    return false;
}

bool isPadActionDown(GameAction action) {
    const GamepadFrame& frame = gamepadFrame();
    if (frame.pad < 0) {
        return false;
    }
    const std::size_t direction = padDirectionFor(action);
    if (direction != PAD_DIRECTION_COUNT) {
        return frame.held[direction];
    }
    for (const GamepadButton button : padBindingFor(action)) {
        if (button != GAMEPAD_BUTTON_UNKNOWN && IsGamepadButtonDown(frame.pad, button)) {
            return true;
        }
    }
    return false;
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
    GamepadFrame& pad = gamepadFrame();
    const bool gamepadUsed = sampleGamepad(pad, GetFrameTime());

    InputState state;
    state.mouse = GetMousePosition();
    const Vector2 delta = GetMouseDelta();
    state.mouseMoved =
        std::fabs(delta.x) > MOUSE_MOVE_EPSILON || std::fabs(delta.y) > MOUSE_MOVE_EPSILON;
    state.mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    state.mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    state.mouseReleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    state.wheel = GetMouseWheelMove();
    state.navigateUp = pressedOrRepeated(KEY_UP) || pad.repeated[PAD_UP];
    state.navigateDown = pressedOrRepeated(KEY_DOWN) || pad.repeated[PAD_DOWN];
    state.navigateLeft = pressedOrRepeated(KEY_LEFT) || pad.repeated[PAD_LEFT];
    state.navigateRight = pressedOrRepeated(KEY_RIGHT) || pad.repeated[PAD_RIGHT];
    state.confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                    isPadButtonPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    state.back = IsKeyPressed(KEY_ESCAPE) || isPadButtonPressed(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    state.backspace = pressedOrRepeated(KEY_BACKSPACE);
    const bool keyPressed = GetKeyPressed() != 0;
    state.anyPressed = keyPressed || state.mousePressed || gamepadUsed;
    readTypedCharacters(state);

    if (gamepadUsed) {
        pad.usedLast = true;
    } else if (keyPressed || state.mousePressed || state.mouseMoved) {
        pad.usedLast = false;
    }
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
    return isPadActionPressed(action);
}

bool isActionDown(GameAction action) {
    for (const KeyboardKey key : bindingFor(action)) {
        if (key != KEY_NULL && IsKeyDown(key)) {
            return true;
        }
    }
    return isPadActionDown(action);
}

bool isUsingGamepad() {
    const GamepadFrame& frame = gamepadFrame();
    return frame.usedLast && frame.pad >= 0;
}

} // namespace app
