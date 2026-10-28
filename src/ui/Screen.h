#pragma once

#include "app/Input.h"

/// @file Screen.h
/// Interface implemented by every screen of the state machine.

namespace ui {

/// One state of the screen state machine (menu, gameplay, result screen...).
class Screen {
public:
    virtual ~Screen() = default;

    /// Called when the screen becomes active through a replace or push.
    virtual void onEnter() {}

    /// Called when a screen pushed on top of this one is popped.
    virtual void onResume() {}

    /// Advances the screen by one frame.
    virtual void update(float deltaSeconds, const app::InputState& input) = 0;

    /// Draws the screen.
    virtual void draw() = 0;

    /// True if the screen below should be drawn first (e.g. result overlays on gameplay).
    virtual bool isOverlay() const { return false; }
};

} // namespace ui
