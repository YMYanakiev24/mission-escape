#include "ui/LoadGameScreen.h"

#include "core/LevelRules.h"
#include "render/Palette.h"
#include "render/Primitives.h"
#include "ui/MenuStyle.h"

#include <raylib.h>

#include <string>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float CARD_WIDTH = 940.0f;
constexpr float CARD_HEIGHT = 124.0f;
constexpr float CARD_GAP = 18.0f;
constexpr float FIRST_CARD_Y = 160.0f;
constexpr float CARD_PADDING = 22.0f;
constexpr float BUTTON_WIDTH = 150.0f;
constexpr float RESUME_WIDTH = 200.0f;
constexpr float BUTTON_HEIGHT = 46.0f;
constexpr float BUTTON_GAP = 12.0f;

std::size_t slotIndex(int index) {
    return static_cast<std::size_t>(index);
}

} // namespace

LoadGameScreen::LoadGameScreen(app::AppContext& context) : m_context(context) {
    m_focus.setFeedback(&context.uiSounds);
    m_confirm.setFeedback(&context.uiSounds);
}

void LoadGameScreen::onEnter() {
    m_confirm.close();
    refresh();
}

void LoadGameScreen::refresh() {
    m_focus.clear();
    for (int index = 0; index < core::SAVE_SLOT_COUNT; ++index) {
        const app::SaveSlot& slot = m_context.slots[slotIndex(index)];
        SlotControls& controls = m_controls[slotIndex(index)];
        const bool ready = slot.status == app::SlotStatus::Ready;
        const bool hasSnapshot = ready && slot.data->snapshot.has_value();
        controls.load.setEnabled(ready);
        controls.resume.setEnabled(hasSnapshot);
        controls.remove.setEnabled(slot.status != app::SlotStatus::Empty);
        if (hasSnapshot) {
            controls.resume.setLabel("Resume Level " + std::to_string(slot.data->snapshot->level));
        }
        for (Button* button : {&controls.load, &controls.resume, &controls.remove}) {
            if (button->isEnabled()) {
                m_focus.add(*button);
            }
        }
    }
    m_focus.add(m_back);
}

Rectangle LoadGameScreen::cardBounds(int index) const {
    return stackedItem(m_context.theme, CARD_WIDTH, CARD_HEIGHT, FIRST_CARD_Y, index, CARD_GAP);
}

void LoadGameScreen::layout() {
    const render::Theme& theme = m_context.theme;
    for (int index = 0; index < core::SAVE_SLOT_COUNT; ++index) {
        const Rectangle card = cardBounds(index);
        SlotControls& controls = m_controls[slotIndex(index)];
        const float height = theme.px(BUTTON_HEIGHT);
        const float y = card.y + card.height - height - theme.px(CARD_PADDING * 0.7f);
        float x = card.x + card.width - theme.px(CARD_PADDING);
        for (Button* button : {&controls.remove, &controls.resume, &controls.load}) {
            const float width = theme.px(button == &controls.resume ? RESUME_WIDTH : BUTTON_WIDTH);
            x -= width;
            button->setBounds(Rectangle{x, y, width, height});
            x -= theme.px(BUTTON_GAP);
        }
    }
    const Rectangle lastCard = cardBounds(core::SAVE_SLOT_COUNT - 1);
    const float width = theme.px(220.0f);
    m_back.setBounds(Rectangle{(static_cast<float>(GetScreenWidth()) - width) / 2.0f,
                               lastCard.y + lastCard.height + theme.px(26.0f), width,
                               theme.px(48.0f)});
}

void LoadGameScreen::handleAction(const Widget* fired) {
    if (fired == &m_back) {
        m_context.navigator.replaceWith(app::ScreenId::MainMenu);
        return;
    }
    for (int index = 0; index < core::SAVE_SLOT_COUNT; ++index) {
        const SlotControls& controls = m_controls[slotIndex(index)];
        const int slot = index + 1;
        if (fired == &controls.load && m_context.activateSlot(slot)) {
            m_context.navigator.replaceWith(app::ScreenId::LevelSelect);
        } else if (fired == &controls.resume && m_context.activateSlot(slot)) {
            if (!m_context.resumeSnapshot()) {
                refresh();
            }
        } else if (fired == &controls.remove) {
            m_pendingDelete = slot;
            const app::SaveSlot& saved = m_context.slots[slotIndex(index)];
            const std::string owner = saved.status == app::SlotStatus::Ready
                                          ? saved.data->playerName + "'s progress"
                                          : "this corrupted save";
            m_confirm.open("Delete slot " + std::to_string(slot) + "?",
                           "This permanently deletes " + owner + ".", "Delete", "Cancel");
        }
    }
}

void LoadGameScreen::update(float deltaSeconds, const app::InputState& input) {
    m_context.menuBackground.update(deltaSeconds);
    layout();
    if (m_confirm.isOpen()) {
        if (m_confirm.update(m_context.theme, input) == DialogResult::Confirmed) {
            m_context.deleteSlot(m_pendingDelete);
            refresh();
        }
        return;
    }
    if (input.back) {
        m_context.navigator.replaceWith(app::ScreenId::MainMenu);
        return;
    }
    handleAction(m_focus.update(input));
}

void LoadGameScreen::drawSlotCard(int index) const {
    const render::Theme& theme = m_context.theme;
    const app::SaveSlot& slot = m_context.slots[slotIndex(index)];
    const Rectangle card = cardBounds(index);
    const bool ready = slot.status == app::SlotStatus::Ready;
    render::drawPanel(card, theme.px(14.0f), render::fade(palette::PANEL, 0.96f),
                      ready ? palette::WALL : render::fade(palette::TEXT_MUTED, 0.4f),
                      theme.px(1.5f));
    const float x = card.x + theme.px(CARD_PADDING);
    const float titleY = card.y + theme.px(CARD_PADDING * 0.8f);
    const std::string prefix = "SLOT " + std::to_string(index + 1) + "   ";
    if (slot.status == app::SlotStatus::Empty) {
        theme.drawText(prefix + "Empty", Vector2{x, titleY}, theme.px(render::textsize::LARGE),
                       palette::TEXT_MUTED);
        return;
    }
    if (slot.status == app::SlotStatus::Corrupt) {
        theme.drawText(prefix + "Save corrupted", Vector2{x, titleY},
                       theme.px(render::textsize::LARGE), palette::WALL_GLOW);
        theme.drawText("The file could not be read (" + slot.error + "). You can delete it.",
                       Vector2{x, titleY + theme.px(40.0f)}, theme.px(render::textsize::SMALL),
                       palette::TEXT_MUTED);
        return;
    }
    const core::SaveData& data = *slot.data;
    theme.drawText(prefix + data.playerName, Vector2{x, titleY}, theme.px(render::textsize::LARGE),
                   palette::TEXT);
    const std::string details = "Level " + std::to_string(data.progress.highestUnlockedLevel()) +
                                "   |   " + std::to_string(data.progress.totalStars()) +
                                " stars   |   " + std::to_string(data.mazeSize) + " x " +
                                std::to_string(data.mazeSize) + "   |   " +
                                std::string(core::viewModeName(data.viewMode));
    theme.drawText(details, Vector2{x, titleY + theme.px(40.0f)}, theme.px(render::textsize::SMALL),
                   palette::ACCENT_GREEN);
    theme.drawText("Last played " + (data.lastPlayed.empty() ? std::string("-") : data.lastPlayed),
                   Vector2{x, titleY + theme.px(66.0f)}, theme.px(render::textsize::TINY),
                   palette::TEXT_MUTED);
}

void LoadGameScreen::draw() {
    const render::Theme& theme = m_context.theme;
    drawMenuBackdrop(m_context);
    drawScreenTitle(theme, "LOAD GAME", "");
    for (int index = 0; index < core::SAVE_SLOT_COUNT; ++index) {
        drawSlotCard(index);
    }
    m_focus.draw(theme, m_context.time);
    drawFooterHint(theme, "ENTER choose     ESC main menu");
    m_confirm.draw(theme, m_context.time);
}

} // namespace ui
