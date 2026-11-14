#include "ui/NewGameScreen.h"

#include "core/Config.h"
#include "core/SaveData.h"
#include "core/Validation.h"
#include "render/Palette.h"
#include "render/Primitives.h"
#include "ui/MenuStyle.h"

#include <raylib.h>

#include <string>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr int STEP_COUNT = 4;
constexpr int SIZE_FIELD_MAX_LENGTH = 4;
constexpr float CONTENT_TOP = 215.0f;
constexpr float WIDE_WIDTH = 600.0f;
constexpr float ROW_HEIGHT = 62.0f;
constexpr float ROW_GAP = 14.0f;
constexpr float NAV_BUTTON_WIDTH = 210.0f;
constexpr float NAV_BUTTON_HEIGHT = 52.0f;
constexpr float NAV_ROW_Y = 520.0f;

bool acceptNameCharacter(char character) {
    return core::isNameCharacter(static_cast<unsigned char>(character));
}

bool acceptSizeCharacter(char character) {
    return (character >= '0' && character <= '9') || character == '-';
}

const char* stepTitle(int step) {
    switch (step) {
        case 0:
            return "Choose a save slot";
        case 1:
            return "Enter your name";
        case 2:
            return "Choose the maze size";
        default:
            return "Choose how you see the maze";
    }
}

} // namespace

NewGameScreen::NewGameScreen(app::AppContext& context)
    : m_context(context),
      m_nameField("Your name", core::PLAYER_NAME_MAX_LENGTH, &acceptNameCharacter),
      m_sizeSlider("Maze size", core::MIN_MAZE_SIZE, core::MAX_MAZE_SIZE, 1),
      m_sizeField("10 - 50", SIZE_FIELD_MAX_LENGTH, &acceptSizeCharacter) {
    m_focus.setFeedback(&context.uiSounds);
    m_confirm.setFeedback(&context.uiSounds);
}

void NewGameScreen::onEnter() {
    m_slot = 1;
    m_nameField.setText("");
    m_nameError.clear();
    m_sizeSlider.setValue(core::DEFAULT_MAZE_SIZE);
    m_sizeField.setText(std::to_string(core::DEFAULT_MAZE_SIZE));
    m_sizeError.clear();
    m_sizeField.setError(false);
    m_mode = core::ViewMode::Classic2D;
    m_confirm.close();
    goTo(Step::Slot);
}

void NewGameScreen::goTo(Step step) {
    m_step = step;
    rebuildFocus();
    switch (step) {
        case Step::Slot:
            m_focus.focus(m_slotButtons[static_cast<std::size_t>(m_slot - 1)]);
            break;
        case Step::Name:
            m_focus.focus(m_nameField);
            break;
        case Step::Size:
            m_focus.focus(m_sizeSlider);
            break;
        case Step::Mode:
            m_focus.focus(m_mode == core::ViewMode::Classic2D ? m_mode2D : m_mode3D);
            break;
    }
}

void NewGameScreen::goBack() {
    switch (m_step) {
        case Step::Slot:
            m_context.navigator.replaceWith(app::ScreenId::MainMenu);
            break;
        case Step::Name:
            goTo(Step::Slot);
            break;
        case Step::Size:
            goTo(Step::Name);
            break;
        case Step::Mode:
            goTo(Step::Size);
            break;
    }
}

void NewGameScreen::rebuildFocus() {
    m_focus.clear();
    switch (m_step) {
        case Step::Slot:
            for (Button& button : m_slotButtons) {
                m_focus.add(button);
            }
            break;
        case Step::Name:
            m_focus.add(m_nameField);
            m_focus.add(m_next);
            break;
        case Step::Size:
            m_focus.add(m_sizeSlider);
            m_focus.add(m_sizeField);
            m_focus.add(m_next);
            break;
        case Step::Mode:
            m_focus.add(m_mode2D);
            m_focus.add(m_mode3D);
            m_focus.add(m_next);
            break;
    }
    m_focus.add(m_back);
    m_next.setLabel(m_step == Step::Mode ? "Start" : "Next");
}

std::string NewGameScreen::slotLabel(int slotIndex) const {
    const core::SaveSlot& slot = m_context.profiles.slots()[static_cast<std::size_t>(slotIndex)];
    const std::string prefix = "Slot " + std::to_string(slotIndex + 1) + "  -  ";
    switch (slot.status) {
        case core::SlotStatus::Empty:
            return prefix + "empty";
        case core::SlotStatus::Corrupt:
            return prefix + "save corrupted (will be replaced)";
        case core::SlotStatus::Ready:
            break;
    }
    return prefix + slot.data->playerName + ", level " +
           std::to_string(slot.data->progress.highestUnlockedLevel()) + " (overwrite)";
}

void NewGameScreen::layout() {
    const render::Theme& theme = m_context.theme;
    for (std::size_t index = 0; index < m_slotButtons.size(); ++index) {
        m_slotButtons[index].setLabel(slotLabel(static_cast<int>(index)));
        m_slotButtons[index].setBounds(stackedItem(theme, WIDE_WIDTH, ROW_HEIGHT, CONTENT_TOP,
                                                   static_cast<int>(index), ROW_GAP));
    }
    m_nameField.setBounds(
        stackedItem(theme, WIDE_WIDTH * 0.8f, ROW_HEIGHT, CONTENT_TOP + 40.0f, 0, ROW_GAP));
    m_sizeSlider.setBounds(stackedItem(theme, WIDE_WIDTH, ROW_HEIGHT, CONTENT_TOP, 0, ROW_GAP));
    Rectangle field = stackedItem(theme, 200.0f, ROW_HEIGHT, CONTENT_TOP, 1, ROW_GAP + 34.0f);
    m_sizeField.setBounds(field);
    const Rectangle modeRow = stackedItem(theme, WIDE_WIDTH, 130.0f, CONTENT_TOP, 0, 0.0f);
    const float gap = theme.px(20.0f);
    const float modeWidth = (modeRow.width - gap) / 2.0f;
    m_mode2D.setBounds(Rectangle{modeRow.x, modeRow.y, modeWidth, modeRow.height});
    m_mode3D.setBounds(
        Rectangle{modeRow.x + modeWidth + gap, modeRow.y, modeWidth, modeRow.height});
    m_mode2D.setStyle(m_mode == core::ViewMode::Classic2D ? ButtonStyle::Primary
                                                          : ButtonStyle::Normal);
    m_mode3D.setStyle(m_mode == core::ViewMode::FirstPerson3D ? ButtonStyle::Primary
                                                              : ButtonStyle::Normal);

    const float buttonWidth = theme.px(NAV_BUTTON_WIDTH);
    const float buttonHeight = theme.px(NAV_BUTTON_HEIGHT);
    const float centerX = static_cast<float>(GetScreenWidth()) / 2.0f;
    const float y = theme.px(NAV_ROW_Y);
    const bool hasNext = m_step != Step::Slot;
    m_back.setBounds(
        Rectangle{hasNext ? centerX - buttonWidth - theme.px(10.0f) : centerX - buttonWidth / 2.0f,
                  y, buttonWidth, buttonHeight});
    m_next.setBounds(Rectangle{centerX + theme.px(10.0f), y, buttonWidth, buttonHeight});
}

void NewGameScreen::update(float deltaSeconds, const app::InputState& input) {
    m_context.menuBackground.update(deltaSeconds);
    layout();
    if (m_confirm.isOpen()) {
        if (m_confirm.update(m_context.theme, input) == DialogResult::Confirmed) {
            goTo(Step::Name);
        }
        return;
    }
    if (input.back) {
        goBack();
        return;
    }
    const int sliderBefore = m_sizeSlider.value();
    const std::string fieldBefore = m_sizeField.text();
    const Widget* fired = m_focus.update(input);
    if (fired == &m_back) {
        goBack();
        return;
    }
    switch (m_step) {
        case Step::Slot:
            handleSlotStep(fired);
            break;
        case Step::Name:
            handleNameStep(fired, input);
            break;
        case Step::Size:
            handleSizeStep(fired, m_sizeSlider.value() != sliderBefore,
                           m_sizeField.text() != fieldBefore);
            break;
        case Step::Mode:
            handleModeStep(fired);
            break;
    }
}

void NewGameScreen::handleSlotStep(const Widget* fired) {
    for (std::size_t index = 0; index < m_slotButtons.size(); ++index) {
        if (fired != &m_slotButtons[index]) {
            continue;
        }
        m_slot = static_cast<int>(index) + 1;
        const core::SaveSlot& slot = m_context.profiles.slots()[index];
        if (slot.status == core::SlotStatus::Empty) {
            goTo(Step::Name);
        } else {
            const std::string owner =
                slot.status == core::SlotStatus::Ready ? slot.data->playerName + "'s" : "the";
            m_confirm.open("Overwrite slot " + std::to_string(m_slot) + "?",
                           "This permanently replaces " + owner +
                               " progress in this slot with a new game.",
                           "Overwrite", "Cancel");
        }
    }
}

void NewGameScreen::confirmName() {
    const core::NameValidation result = core::validatePlayerName(m_nameField.text());
    m_nameField.setError(!result.valid);
    m_nameError = result.message;
    if (result.valid) {
        m_nameField.setText(result.name);
        goTo(Step::Size);
    }
}

void NewGameScreen::handleNameStep(const Widget* fired, const app::InputState& input) {
    const bool enterInField = input.confirm && m_focus.focused() == &m_nameField;
    if (fired == &m_next || enterInField) {
        confirmName();
    } else if (fired == &m_nameField) {
        m_nameError.clear(); // typing clears the previous complaint
        m_nameField.setError(false);
    }
}

void NewGameScreen::handleSizeStep(const Widget* fired, bool sliderMoved, bool fieldEdited) {
    if (sliderMoved) {
        m_sizeField.setText(std::to_string(m_sizeSlider.value()));
    }
    const core::SizeValidation result = core::validateMazeSize(m_sizeField.text());
    if (fieldEdited && result.valid) {
        m_sizeSlider.setValue(result.size);
    }
    m_sizeError = result.message;
    m_sizeField.setError(!result.valid);
    m_next.setEnabled(result.valid);
    if (fired == &m_next && result.valid) {
        goTo(Step::Mode);
    }
}

void NewGameScreen::handleModeStep(const Widget* fired) {
    if (fired == &m_mode2D) {
        m_mode = core::ViewMode::Classic2D;
    } else if (fired == &m_mode3D) {
        m_mode = core::ViewMode::FirstPerson3D;
    } else if (fired == &m_next) {
        finish();
    }
}

void NewGameScreen::finish() {
    const core::NameValidation name = core::validatePlayerName(m_nameField.text());
    const core::SizeValidation size = core::validateMazeSize(m_sizeField.text());
    if (!name.valid || !size.valid) {
        goTo(name.valid ? Step::Size : Step::Name);
        return;
    }
    core::SaveData data;
    data.playerName = name.name;
    data.mazeSize = size.size;
    data.viewMode = m_mode;
    m_context.profiles.createProfile(m_slot, std::move(data));
    m_context.navigator.replaceWith(app::ScreenId::LevelSelect);
}

void NewGameScreen::drawStepHeader() const {
    const int stepIndex = static_cast<int>(m_step);
    drawScreenTitle(m_context.theme, "NEW GAME",
                    "Step " + std::to_string(stepIndex + 1) + " of " + std::to_string(STEP_COUNT) +
                        "  -  " + stepTitle(stepIndex));
}

void NewGameScreen::drawStepDetails() const {
    const render::Theme& theme = m_context.theme;
    const float size = theme.px(render::textsize::SMALL);
    const float centerX = static_cast<float>(GetScreenWidth()) / 2.0f;
    if (m_step == Step::Name) {
        const Rectangle field = m_nameField.bounds();
        const std::string& message =
            m_nameError.empty() ? std::string("1-16 characters: letters, digits and spaces")
                                : m_nameError;
        theme.drawTextCentered(message, Vector2{centerX, field.y + field.height + theme.px(26.0f)},
                               size,
                               m_nameError.empty() ? palette::TEXT_MUTED : palette::WALL_GLOW);
    } else if (m_step == Step::Size) {
        const Rectangle field = m_sizeField.bounds();
        theme.drawTextRight(
            "or type it:", Vector2{field.x - theme.px(14.0f), field.y + theme.px(18.0f)}, size,
            palette::TEXT_MUTED);
        const int value = m_sizeSlider.value();
        const std::string hint = m_sizeError.empty()
                                     ? std::to_string(value) + " x " + std::to_string(value) +
                                           " cells. Start top-left, exit bottom-right."
                                     : m_sizeError;
        theme.drawTextCentered(hint, Vector2{centerX, field.y + field.height + theme.px(28.0f)},
                               size,
                               m_sizeError.empty() ? palette::TEXT_MUTED : palette::WALL_GLOW);
    } else if (m_step == Step::Mode) {
        const Rectangle row = m_mode2D.bounds();
        const std::string description =
            m_mode == core::ViewMode::Classic2D
                ? "Top-down neon view of the whole maze. Arrow keys or WASD move."
                : "Walk the corridors in first person with a minimap. Turn with left/right.";
        theme.drawTextCentered(description, Vector2{centerX, row.y + row.height + theme.px(36.0f)},
                               size, palette::TEXT_MUTED);
        theme.drawTextCentered("You can switch the view later in Level Select.",
                               Vector2{centerX, row.y + row.height + theme.px(64.0f)}, size,
                               render::fade(palette::TEXT_MUTED, 0.7f));
    }
}

void NewGameScreen::draw() {
    drawMenuBackdrop(m_context);
    drawStepHeader();
    m_focus.draw(m_context.theme, m_context.time);
    drawStepDetails();
    drawFooterHint(m_context.theme, "ENTER confirm     ESC previous step");
    m_confirm.draw(m_context.theme, m_context.time);
}

} // namespace ui
