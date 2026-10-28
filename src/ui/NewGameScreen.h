#pragma once

#include "app/AppContext.h"
#include "core/LevelRules.h"
#include "ui/ConfirmDialog.h"
#include "ui/Screen.h"
#include "ui/Widgets.h"

#include <array>
#include <string>

/// @file NewGameScreen.h
/// Four-step wizard: save slot, player name, maze size, view mode.

namespace ui {

/// Creates a new profile. Every step validates its input before moving on.
class NewGameScreen : public Screen {
public:
    /// Creates the screen; @p context must outlive it.
    explicit NewGameScreen(app::AppContext& context);

    /// Resets the screen each time it becomes active.
    void onEnter() override;
    /// Handles input and advances the screen by one frame.
    void update(float deltaSeconds, const app::InputState& input) override;
    /// Draws the screen.
    void draw() override;

private:
    enum class Step { Slot, Name, Size, Mode };

    void goTo(Step step);
    void goBack();
    void rebuildFocus();
    void layout();
    void handleSlotStep(const Widget* fired);
    void handleNameStep(const Widget* fired, const app::InputState& input);
    void handleSizeStep(const Widget* fired, bool sliderMoved, bool fieldEdited);
    void handleModeStep(const Widget* fired);
    void confirmName();
    void finish();
    std::string slotLabel(int slotIndex) const;
    void drawStepHeader() const;
    void drawStepDetails() const;

    app::AppContext& m_context;
    Step m_step = Step::Slot;
    int m_slot = 1;
    std::array<Button, core::SAVE_SLOT_COUNT> m_slotButtons;
    TextField m_nameField;
    std::string m_nameError;
    Slider m_sizeSlider;
    TextField m_sizeField;
    std::string m_sizeError;
    Button m_mode2D{"2D Classic"};
    Button m_mode3D{"3D First-Person"};
    core::ViewMode m_mode = core::ViewMode::Classic2D;
    Button m_next{"Next", ButtonStyle::Primary};
    Button m_back{"Back"};
    FocusGroup m_focus;
    ConfirmDialog m_confirm;
};

} // namespace ui
