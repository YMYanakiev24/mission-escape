#pragma once

#include "app/AppContext.h"
#include "core/Config.h"
#include "ui/ConfirmDialog.h"
#include "ui/Screen.h"
#include "ui/Widgets.h"

#include <array>

/// @file LoadGameScreen.h
/// Lists the three save slots with Load, Resume and Delete actions.

namespace ui {

/// Shows every slot's name, last-played date, progress, size and mode.
class LoadGameScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit LoadGameScreen(app::AppContext& context);

    /// Resets the screen each time it becomes active.
    void onEnter() override;
    /// Handles input and advances the screen by one frame.
    void update(float deltaSeconds, const app::InputState& input) override;
    /// Draws the screen.
    void draw() override;

private:
    /// The buttons that belong to one slot card.
    struct SlotControls {
        Button load{"Load", ButtonStyle::Primary};
        Button resume{"Resume"};
        Button remove{"Delete"};
    };

    void refresh();
    void layout();
    Rectangle cardBounds(int index) const;
    void handleAction(const Widget* fired);
    void drawSlotCard(int index) const;

    app::AppContext& m_context;
    std::array<SlotControls, core::SAVE_SLOT_COUNT> m_controls;
    Button m_back{"Back"};
    FocusGroup m_focus;
    ConfirmDialog m_confirm;
    int m_pendingDelete = 0;
};

} // namespace ui
