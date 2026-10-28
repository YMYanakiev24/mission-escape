#pragma once

#include "app/Input.h"
#include "render/Theme.h"

#include <raylib.h>

#include <string>
#include <utility>
#include <vector>

/// @file Widgets.h
/// Retained-mode UI widgets (Button, Slider, Toggle, TextField) and the FocusGroup that makes
/// every menu usable with both mouse and keyboard (arrows + Enter).

namespace ui {

/// Receives notifications for UI sound effects.
class UiFeedback {
public:
    virtual ~UiFeedback() = default;
    /// Keyboard or mouse focus moved to another widget.
    virtual void focusMoved() = 0;
    /// A widget was activated (clicked, toggled, confirmed).
    virtual void activated() = 0;
};

/// Base class: a rectangular, focusable control.
class Widget {
public:
    virtual ~Widget() = default;

    /// Places the widget on screen.
    void setBounds(Rectangle bounds) noexcept { m_bounds = bounds; }
    /// Screen rectangle of the widget.
    Rectangle bounds() const noexcept { return m_bounds; }
    /// Disabled widgets are greyed out and skipped by keyboard navigation.
    void setEnabled(bool enabled) noexcept { m_enabled = enabled; }
    /// True when the widget can be focused and used.
    bool isEnabled() const noexcept { return m_enabled; }

    /// True when @p point lies inside the widget.
    bool contains(Vector2 point) const;

    /// Processes this frame's input; returns true when the widget fired (click, value change).
    virtual bool handleInput(const app::InputState& input, bool focused) = 0;

    /// Draws the widget; @p time drives small animations such as the caret blink.
    virtual void draw(const render::Theme& theme, bool focused, float time) const = 0;

    /// True if Left/Right adjust this widget instead of moving the focus.
    virtual bool usesHorizontalKeys() const { return false; }

    /// True for widgets that fire every frame while dragged (no click sound while dragging).
    virtual bool firesContinuously() const { return false; }

protected:
    Rectangle m_bounds{};
    bool m_enabled = true;
};

/// Visual emphasis of a button.
enum class ButtonStyle { Normal, Primary };

/// Clickable push button.
class Button : public Widget {
public:
    /// Creates a button with a label and visual style.
    explicit Button(std::string label = {}, ButtonStyle style = ButtonStyle::Normal);

    /// Changes the displayed text.
    void setLabel(std::string label) { m_label = std::move(label); }
    /// The displayed text.
    const std::string& label() const noexcept { return m_label; }
    /// Changes the visual emphasis.
    void setStyle(ButtonStyle style) noexcept { m_style = style; }

    /// Reacts to this frame's input; returns true when the widget fired.
    bool handleInput(const app::InputState& input, bool focused) override;
    /// Draws the widget (highlighted when focused).
    void draw(const render::Theme& theme, bool focused, float time) const override;

private:
    std::string m_label;
    ButtonStyle m_style;
};

/// Horizontal integer slider with a label and value read-out.
class Slider : public Widget {
public:
    /// Creates a slider for values in [minimum, maximum] moving by @p step per key press.
    Slider(std::string label, int minimum, int maximum, int step);

    /// Current value.
    int value() const noexcept { return m_value; }
    /// Sets the value, clamped to the slider range.
    void setValue(int value);
    /// Changes the displayed text.
    void setLabel(std::string label) { m_label = std::move(label); }

    /// Reacts to this frame's input; returns true when the widget fired.
    bool handleInput(const app::InputState& input, bool focused) override;
    /// Draws the widget (highlighted when focused).
    void draw(const render::Theme& theme, bool focused, float time) const override;
    /// Left/Right change the value instead of moving the focus.
    bool usesHorizontalKeys() const override { return true; }
    /// Dragging changes the value continuously.
    bool firesContinuously() const override { return true; }

private:
    Rectangle trackBounds() const;

    std::string m_label;
    int m_minimum;
    int m_maximum;
    int m_step;
    int m_value;
    bool m_dragging = false;
};

/// On/off switch with a label.
class Toggle : public Widget {
public:
    /// Creates a labelled on/off switch.
    explicit Toggle(std::string label, bool value = false);

    /// True when switched on.
    bool value() const noexcept { return m_value; }
    /// Switches on or off without firing.
    void setValue(bool value) noexcept { m_value = value; }

    /// Reacts to this frame's input; returns true when the widget fired.
    bool handleInput(const app::InputState& input, bool focused) override;
    /// Draws the widget (highlighted when focused).
    void draw(const render::Theme& theme, bool focused, float time) const override;
    /// Left/Right change the value instead of moving the focus.
    bool usesHorizontalKeys() const override { return true; }

private:
    std::string m_label;
    bool m_value;
};

/// Single-line text input. Characters are filtered as they are typed.
class TextField : public Widget {
public:
    /// Decides whether a typed character is accepted.
    using CharacterFilter = bool (*)(char);

    /// Creates an empty field showing @p placeholder, accepting at most @p maxLength filtered
    /// characters.
    TextField(std::string placeholder, int maxLength, CharacterFilter filter);

    /// Current content.
    const std::string& text() const noexcept { return m_text; }
    /// Replaces the content (truncated to the maximum length).
    void setText(std::string text);
    /// Highlights the field in an error state (e.g. invalid size).
    void setError(bool error) noexcept { m_error = error; }

    /// Reacts to this frame's input; returns true when the widget fired.
    bool handleInput(const app::InputState& input, bool focused) override;
    /// Draws the widget (highlighted when focused).
    void draw(const render::Theme& theme, bool focused, float time) const override;

private:
    std::string m_placeholder;
    std::string m_text;
    int m_maxLength;
    CharacterFilter m_filter;
    bool m_error = false;
};

/// Keyboard/mouse focus management over an ordered set of widgets.
///
/// Arrow keys move the focus spatially (to the nearest widget in that direction, wrapping
/// vertically); the mouse focuses whatever it hovers. Widgets are referenced, not owned.
class FocusGroup {
public:
    /// Removes all widgets (call when rebuilding a screen's layout).
    void clear();

    /// Appends a widget; the first enabled widget receives focus by default.
    void add(Widget& widget);

    /// Updates focus and forwards input; returns the widget that fired this frame, if any.
    Widget* update(const app::InputState& input);

    /// Draws all widgets.
    void draw(const render::Theme& theme, float time) const;

    /// Focuses @p widget if it belongs to the group.
    void focus(const Widget& widget);

    /// The focused widget, or nullptr.
    Widget* focused() const;

    /// Sets the receiver of focus/activation sound notifications (may be null).
    void setFeedback(UiFeedback* feedback) noexcept { m_feedback = feedback; }

private:
    void moveFocus(float directionX, float directionY);
    void focusHovered(Vector2 mouse);
    void setFocusIndex(int index);
    bool isFocusable(int index) const;

    std::vector<Widget*> m_widgets;
    int m_focus = -1;
    UiFeedback* m_feedback = nullptr;
};

} // namespace ui
