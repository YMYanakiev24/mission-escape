#include "ui/Widgets.h"

#include "render/Palette.h"
#include "render/Primitives.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float CORNER_RADIUS = 10.0f;
constexpr float BORDER_THICKNESS = 2.0f;
constexpr float FOCUS_BORDER_THICKNESS = 3.0f;
constexpr float LABEL_PADDING = 18.0f;
constexpr float CARET_BLINK_HZ = 1.2f;
constexpr float FOCUS_PULSE_HZ = 1.5f;
constexpr float MIN_NAVIGATION_DISTANCE = 1.0f;
constexpr float CROSS_AXIS_WEIGHT = 2.0f;

Vector2 centerOf(Rectangle bounds) {
    return Vector2{bounds.x + bounds.width / 2.0f, bounds.y + bounds.height / 2.0f};
}

/// Common rounded background used by every widget; the focus state glows green.
void drawWidgetFrame(const render::Theme& theme, Rectangle bounds, bool focused, bool enabled,
                     Color idleBorder, float time) {
    const Color fill = focused ? palette::PANEL_HI : palette::PANEL;
    Color border = focused ? palette::ACCENT_GREEN : idleBorder;
    if (!enabled) {
        border = render::fade(palette::TEXT_MUTED, 0.35f);
    }
    if (focused) {
        const float glow = 0.25f + 0.2f * render::pulse(time, FOCUS_PULSE_HZ);
        const Rectangle halo{bounds.x - theme.px(4.0f), bounds.y - theme.px(4.0f),
                             bounds.width + theme.px(8.0f), bounds.height + theme.px(8.0f)};
        render::drawPanel(halo, theme.px(CORNER_RADIUS + 4.0f),
                          render::fade(palette::ACCENT_GREEN, glow * 0.35f), Color{0, 0, 0, 0},
                          0.0f);
    }
    const float thickness = theme.px(focused ? FOCUS_BORDER_THICKNESS : BORDER_THICKNESS);
    render::drawPanel(bounds, theme.px(CORNER_RADIUS), fill, border, thickness);
}

Color labelColor(bool enabled) {
    return enabled ? palette::TEXT : render::fade(palette::TEXT_MUTED, 0.6f);
}

} // namespace

bool Widget::contains(Vector2 point) const {
    return CheckCollisionPointRec(point, m_bounds);
}

// ------------------------------------------------------------------------------------------
// Button
// ------------------------------------------------------------------------------------------

Button::Button(std::string label, ButtonStyle style) : m_label(std::move(label)), m_style(style) {}

bool Button::handleInput(const app::InputState& input, bool focused) {
    if (!m_enabled) {
        return false;
    }
    const bool clicked = input.mousePressed && contains(input.mouse);
    return clicked || (focused && input.confirm);
}

void Button::draw(const render::Theme& theme, bool focused, float time) const {
    const Color idle = m_style == ButtonStyle::Primary ? palette::ACCENT_GREEN_DIM : palette::WALL;
    drawWidgetFrame(theme, m_bounds, focused && m_enabled, m_enabled, idle, time);
    const float size = std::min(theme.px(render::textsize::BODY), m_bounds.height * 0.55f);
    theme.drawTextCentered(m_label, centerOf(m_bounds), size, labelColor(m_enabled));
}

// ------------------------------------------------------------------------------------------
// Slider
// ------------------------------------------------------------------------------------------

Slider::Slider(std::string label, int minimum, int maximum, int step)
    : m_label(std::move(label)), m_minimum(minimum), m_maximum(maximum), m_step(std::max(1, step)),
      m_value(minimum) {}

void Slider::setValue(int value) {
    m_value = std::clamp(value, m_minimum, m_maximum);
}

Rectangle Slider::trackBounds() const {
    // Label on the left 40%, track in the middle, value read-out on the right.
    const float trackX = m_bounds.x + m_bounds.width * 0.42f;
    const float trackWidth = m_bounds.width * 0.42f;
    const float trackHeight = std::max(4.0f, m_bounds.height * 0.14f);
    return Rectangle{trackX, m_bounds.y + (m_bounds.height - trackHeight) / 2.0f, trackWidth,
                     trackHeight};
}

bool Slider::handleInput(const app::InputState& input, bool focused) {
    if (!m_enabled) {
        return false;
    }
    const int before = m_value;
    if (focused && input.navigateLeft) {
        setValue(m_value - m_step);
    }
    if (focused && input.navigateRight) {
        setValue(m_value + m_step);
    }
    if (input.mousePressed && contains(input.mouse)) {
        m_dragging = true;
    }
    if (!input.mouseDown) {
        m_dragging = false;
    }
    if (m_dragging) {
        const Rectangle track = trackBounds();
        const float ratio = std::clamp((input.mouse.x - track.x) / track.width, 0.0f, 1.0f);
        setValue(m_minimum +
                 static_cast<int>(std::lround(ratio * static_cast<float>(m_maximum - m_minimum))));
    }
    return m_value != before;
}

void Slider::draw(const render::Theme& theme, bool focused, float time) const {
    drawWidgetFrame(theme, m_bounds, focused && m_enabled, m_enabled, palette::WALL, time);
    const float size = std::min(theme.px(render::textsize::BODY), m_bounds.height * 0.5f);
    const Vector2 labelExtent = theme.measure(m_label, size);
    theme.drawText(m_label,
                   Vector2{m_bounds.x + theme.px(LABEL_PADDING),
                           m_bounds.y + (m_bounds.height - labelExtent.y) / 2.0f},
                   size, labelColor(m_enabled));

    const Rectangle track = trackBounds();
    const float range = static_cast<float>(std::max(1, m_maximum - m_minimum));
    const float ratio = static_cast<float>(m_value - m_minimum) / range;
    DrawRectangleRounded(track, 1.0f, 6, palette::BG_DEEP);
    DrawRectangleRounded(Rectangle{track.x, track.y, track.width * ratio, track.height}, 1.0f, 6,
                         focused ? palette::ACCENT_GREEN : palette::ACCENT_GREEN_DIM);
    const Vector2 knob{track.x + track.width * ratio, track.y + track.height / 2.0f};
    const float knobRadius = track.height * 1.6f;
    if (focused) {
        render::drawGlow(knob, knobRadius * 2.6f, render::fade(palette::ACCENT_GREEN, 0.5f));
    }
    DrawCircleV(knob, knobRadius, palette::TEXT);

    const std::string valueText = std::to_string(m_value);
    theme.drawTextRight(valueText,
                        Vector2{m_bounds.x + m_bounds.width - theme.px(LABEL_PADDING),
                                m_bounds.y + (m_bounds.height - labelExtent.y) / 2.0f},
                        size, focused ? palette::ACCENT_GREEN : palette::TEXT_MUTED);
}

// ------------------------------------------------------------------------------------------
// Toggle
// ------------------------------------------------------------------------------------------

Toggle::Toggle(std::string label, bool value) : m_label(std::move(label)), m_value(value) {}

bool Toggle::handleInput(const app::InputState& input, bool focused) {
    if (!m_enabled) {
        return false;
    }
    const bool clicked = input.mousePressed && contains(input.mouse);
    const bool keyed = focused && (input.confirm || input.navigateLeft || input.navigateRight);
    if (clicked || keyed) {
        m_value = !m_value;
        return true;
    }
    return false;
}

void Toggle::draw(const render::Theme& theme, bool focused, float time) const {
    drawWidgetFrame(theme, m_bounds, focused && m_enabled, m_enabled, palette::WALL, time);
    const float size = std::min(theme.px(render::textsize::BODY), m_bounds.height * 0.5f);
    const Vector2 labelExtent = theme.measure(m_label, size);
    theme.drawText(m_label,
                   Vector2{m_bounds.x + theme.px(LABEL_PADDING),
                           m_bounds.y + (m_bounds.height - labelExtent.y) / 2.0f},
                   size, labelColor(m_enabled));

    const float pillHeight = m_bounds.height * 0.46f;
    const float pillWidth = pillHeight * 2.0f;
    const Rectangle pill{m_bounds.x + m_bounds.width - theme.px(LABEL_PADDING) - pillWidth,
                         m_bounds.y + (m_bounds.height - pillHeight) / 2.0f, pillWidth, pillHeight};
    DrawRectangleRounded(pill, 1.0f, 8, m_value ? palette::ACCENT_GREEN_DIM : palette::BG_DEEP);
    DrawRectangleRoundedLinesEx(pill, 1.0f, 8, theme.px(1.5f),
                                m_value ? palette::ACCENT_GREEN : palette::TEXT_MUTED);
    const float knobRadius = pillHeight * 0.36f;
    const float knobX =
        m_value ? pill.x + pill.width - pillHeight / 2.0f : pill.x + pillHeight / 2.0f;
    DrawCircleV(Vector2{knobX, pill.y + pillHeight / 2.0f}, knobRadius,
                m_value ? palette::TEXT : palette::TEXT_MUTED);
    theme.drawTextRight(
        m_value ? "ON" : "OFF",
        Vector2{pill.x - theme.px(10.0f), m_bounds.y + (m_bounds.height - labelExtent.y) / 2.0f},
        size, m_value ? palette::ACCENT_GREEN : palette::TEXT_MUTED);
}

// ------------------------------------------------------------------------------------------
// TextField
// ------------------------------------------------------------------------------------------

TextField::TextField(std::string placeholder, int maxLength, CharacterFilter filter)
    : m_placeholder(std::move(placeholder)), m_maxLength(maxLength), m_filter(filter) {}

void TextField::setText(std::string text) {
    m_text = std::move(text);
    if (static_cast<int>(m_text.size()) > m_maxLength) {
        m_text.resize(static_cast<std::size_t>(m_maxLength));
    }
}

bool TextField::handleInput(const app::InputState& input, bool focused) {
    if (!m_enabled || !focused) {
        return false;
    }
    const std::string before = m_text;
    for (const char character : input.typed) {
        const bool fits = static_cast<int>(m_text.size()) < m_maxLength;
        if (fits && (m_filter == nullptr || m_filter(character))) {
            m_text.push_back(character);
        }
    }
    if (input.backspace && !m_text.empty()) {
        m_text.pop_back();
    }
    return m_text != before;
}

void TextField::draw(const render::Theme& theme, bool focused, float time) const {
    const Color idle = m_error ? palette::WALL_GLOW : palette::WALL;
    drawWidgetFrame(theme, m_bounds, focused && m_enabled, m_enabled, idle, time);
    const float size = std::min(theme.px(render::textsize::LARGE), m_bounds.height * 0.55f);
    const bool showPlaceholder = m_text.empty();
    const std::string& shown = showPlaceholder ? m_placeholder : m_text;
    const Vector2 extent = theme.measure(shown.empty() ? std::string("M") : shown, size);
    const Vector2 position{m_bounds.x + theme.px(LABEL_PADDING),
                           m_bounds.y + (m_bounds.height - extent.y) / 2.0f};
    theme.drawText(shown, position, size,
                   showPlaceholder ? render::fade(palette::TEXT_MUTED, 0.7f) : palette::TEXT);
    const bool caretVisible = focused && render::pulse(time, CARET_BLINK_HZ) > 0.5f;
    if (caretVisible) {
        const float caretX = position.x + (showPlaceholder ? 0.0f : extent.x) + theme.px(3.0f);
        DrawRectangleRec(Rectangle{caretX, position.y, theme.px(2.5f), extent.y},
                         palette::ACCENT_GREEN);
    }
}

// ------------------------------------------------------------------------------------------
// FocusGroup
// ------------------------------------------------------------------------------------------

void FocusGroup::clear() {
    m_widgets.clear(); // keep m_focus so a rebuilt layout keeps the same focused position
}

void FocusGroup::add(Widget& widget) {
    m_widgets.push_back(&widget);
}

bool FocusGroup::isFocusable(int index) const {
    return index >= 0 && index < static_cast<int>(m_widgets.size()) &&
           m_widgets[static_cast<std::size_t>(index)]->isEnabled();
}

Widget* FocusGroup::focused() const {
    return isFocusable(m_focus) ? m_widgets[static_cast<std::size_t>(m_focus)] : nullptr;
}

void FocusGroup::focus(const Widget& widget) {
    for (std::size_t index = 0; index < m_widgets.size(); ++index) {
        if (m_widgets[index] == &widget) {
            m_focus = static_cast<int>(index);
        }
    }
}

void FocusGroup::setFocusIndex(int index) {
    if (index != m_focus && isFocusable(index)) {
        m_focus = index;
        if (m_feedback != nullptr) {
            m_feedback->focusMoved();
        }
    }
}

void FocusGroup::focusHovered(Vector2 mouse) {
    for (std::size_t index = 0; index < m_widgets.size(); ++index) {
        if (m_widgets[index]->isEnabled() && m_widgets[index]->contains(mouse)) {
            setFocusIndex(static_cast<int>(index));
            return;
        }
    }
}

void FocusGroup::moveFocus(float directionX, float directionY) {
    const Widget* current = focused();
    if (current == nullptr) {
        return;
    }
    const Vector2 origin = centerOf(current->bounds());
    int best = -1;
    int wrap = -1;
    float bestScore = std::numeric_limits<float>::max();
    float wrapScore = std::numeric_limits<float>::max();
    for (std::size_t index = 0; index < m_widgets.size(); ++index) {
        if (static_cast<int>(index) == m_focus || !m_widgets[index]->isEnabled()) {
            continue;
        }
        const Vector2 target = centerOf(m_widgets[index]->bounds());
        const float along = (target.x - origin.x) * directionX + (target.y - origin.y) * directionY;
        const float across = std::fabs((target.x - origin.x) * directionY) +
                             std::fabs((target.y - origin.y) * directionX);
        const float score = along + across * CROSS_AXIS_WEIGHT;
        if (along > MIN_NAVIGATION_DISTANCE && score < bestScore) {
            best = static_cast<int>(index);
            bestScore = score;
        } else if (along < -MIN_NAVIGATION_DISTANCE && score < wrapScore) {
            wrap = static_cast<int>(index); // farthest widget on the opposite side
            wrapScore = score;
        }
    }
    const bool vertical = directionY != 0.0f;
    setFocusIndex(best >= 0 ? best : (vertical ? wrap : -1));
}

Widget* FocusGroup::update(const app::InputState& input) {
    if (m_widgets.empty()) {
        return nullptr;
    }
    if (!isFocusable(m_focus)) {
        m_focus = -1;
        for (std::size_t index = 0; index < m_widgets.size() && m_focus < 0; ++index) {
            m_focus = m_widgets[index]->isEnabled() ? static_cast<int>(index) : -1;
        }
    }
    if (input.mouseMoved || input.mousePressed) {
        focusHovered(input.mouse);
    }
    const Widget* current = focused();
    const bool horizontalKeysTaken = current != nullptr && current->usesHorizontalKeys();
    if (input.navigateUp) {
        moveFocus(0.0f, -1.0f);
    } else if (input.navigateDown) {
        moveFocus(0.0f, 1.0f);
    } else if (input.navigateLeft && !horizontalKeysTaken) {
        moveFocus(-1.0f, 0.0f);
    } else if (input.navigateRight && !horizontalKeysTaken) {
        moveFocus(1.0f, 0.0f);
    }
    Widget* fired = nullptr;
    for (std::size_t index = 0; index < m_widgets.size(); ++index) {
        const bool isFocused = static_cast<int>(index) == m_focus;
        if (m_widgets[index]->handleInput(input, isFocused) && fired == nullptr) {
            fired = m_widgets[index];
        }
    }
    const bool dragging =
        fired != nullptr && fired->firesContinuously() && input.mouseDown && !input.mousePressed;
    if (fired != nullptr && m_feedback != nullptr && !dragging) {
        m_feedback->activated();
    }
    return fired;
}

void FocusGroup::draw(const render::Theme& theme, float time) const {
    for (std::size_t index = 0; index < m_widgets.size(); ++index) {
        m_widgets[index]->draw(theme, static_cast<int>(index) == m_focus, time);
    }
}

} // namespace ui
