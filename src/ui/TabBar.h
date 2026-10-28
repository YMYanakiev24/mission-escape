#pragma once

#include "ui/Widgets.h"

#include <raylib.h>

#include <array>
#include <cstddef>
#include <string>

/// @file TabBar.h
/// A row of tab buttons; the tab under the keyboard focus becomes the active one.

namespace ui {

/// Fixed-size row of tabs used by the Help and Settings screens.
template <std::size_t TabCount>
class TabBar {
public:
    /// Creates the tab buttons with the given labels.
    explicit TabBar(const std::array<std::string, TabCount>& labels) {
        for (std::size_t index = 0; index < TabCount; ++index) {
            m_buttons[index].setLabel(labels[index]);
        }
    }

    /// Adds the tab buttons to a focus group.
    void addTo(FocusGroup& group) {
        for (Button& button : m_buttons) {
            group.add(button);
        }
    }

    /// Lays the tabs out evenly across @p row.
    void layout(Rectangle row, float gap) {
        const float width =
            (row.width - gap * static_cast<float>(TabCount - 1)) / static_cast<float>(TabCount);
        for (std::size_t index = 0; index < TabCount; ++index) {
            m_buttons[index].setBounds(Rectangle{row.x + static_cast<float>(index) * (width + gap),
                                                 row.y, width, row.height});
        }
    }

    /// Follows the focus (and clicks) so arrows switch tabs immediately.
    void update(const FocusGroup& group, const Widget* fired) {
        for (std::size_t index = 0; index < TabCount; ++index) {
            if (group.focused() == &m_buttons[index] || fired == &m_buttons[index]) {
                m_active = index;
            }
        }
        for (std::size_t index = 0; index < TabCount; ++index) {
            m_buttons[index].setStyle(index == m_active ? ButtonStyle::Primary
                                                        : ButtonStyle::Normal);
        }
    }

    /// Index of the active tab.
    std::size_t active() const noexcept { return m_active; }
    /// Activates a tab (out-of-range indices select the first tab).
    void setActive(std::size_t index) noexcept { m_active = index < TabCount ? index : 0; }
    /// The button of tab @p index.
    Button& button(std::size_t index) { return m_buttons[index]; }

private:
    std::array<Button, TabCount> m_buttons;
    std::size_t m_active = 0;
};

} // namespace ui
