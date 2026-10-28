#pragma once

#include "app/Input.h"
#include "render/Theme.h"
#include "ui/Widgets.h"

#include <string>

/// @file ConfirmDialog.h
/// Modal yes/no dialog used before destructive actions (overwrite, delete, quit level).

namespace ui {

/// Outcome of one frame of a confirmation dialog.
enum class DialogResult { None, Confirmed, Cancelled };

/// Modal dialog with a title, a message and two buttons. While open it captures all input.
class ConfirmDialog {
public:
    ConfirmDialog();
    ConfirmDialog(const ConfirmDialog&) = delete; // the focus group points at member buttons
    ConfirmDialog& operator=(const ConfirmDialog&) = delete;

    /// Shows the dialog; the cancel button is focused by default to avoid accidents.
    void open(std::string title, std::string message, std::string confirmLabel = "Yes",
              std::string cancelLabel = "No");

    /// Hides the dialog.
    void close() noexcept { m_open = false; }

    /// True while the dialog is visible.
    bool isOpen() const noexcept { return m_open; }

    /// Handles input; Esc cancels. Returns the decision taken this frame, if any.
    DialogResult update(const render::Theme& theme, const app::InputState& input);

    /// Draws the dimmed backdrop and the dialog.
    void draw(const render::Theme& theme, float time) const;

    /// Forwards UI sound notifications.
    void setFeedback(UiFeedback* feedback) { m_focus.setFeedback(feedback); }

private:
    Rectangle panelBounds(const render::Theme& theme) const;
    void layout(const render::Theme& theme);

    bool m_open = false;
    std::string m_title;
    std::string m_message;
    Button m_confirm;
    Button m_cancel;
    FocusGroup m_focus;
};

} // namespace ui
