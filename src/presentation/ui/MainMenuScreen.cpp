#include "ui/MainMenuScreen.h"

#include "core/Validation.h"
#include "render/Palette.h"
#include "render/Primitives.h"
#include "ui/MenuStyle.h"

#include <raylib.h>

#include <algorithm>
#include <string>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float BUTTON_WIDTH = 340.0f;
constexpr float BUTTON_HEIGHT = 52.0f;
constexpr float BUTTON_GAP = 12.0f;
constexpr float FIRST_BUTTON_Y = 300.0f;
constexpr float TITLE_Y = 150.0f;
constexpr float NOTICE_SECONDS = 6.0f;

} // namespace

MainMenuScreen::MainMenuScreen(app::AppContext& context) : m_context(context) {
    m_focus.setFeedback(&context.uiSounds);
    for (Button* button : {&m_continue, &m_newGame, &m_loadGame, &m_settings, &m_help, &m_quit}) {
        m_focus.add(*button);
    }
}

void MainMenuScreen::onEnter() {
    // Problems reported while saving or resuming are shown here for a few seconds.
    m_notice = m_context.profiles.lastError();
    m_noticeSeconds = m_notice.empty() ? 0.0f : NOTICE_SECONDS;
    m_context.profiles.lastError().clear();
    m_continue.setEnabled(canContinue());
    m_focus.focus(canContinue() ? m_continue : m_newGame);
}

bool MainMenuScreen::canContinue() const {
    const int slot = m_context.profiles.settings().lastSlot;
    return core::isValidSlot(slot) &&
           m_context.profiles.slots()[static_cast<std::size_t>(slot - 1)].status ==
               core::SlotStatus::Ready;
}

void MainMenuScreen::layout() {
    int index = 0;
    for (Button* button : {&m_continue, &m_newGame, &m_loadGame, &m_settings, &m_help, &m_quit}) {
        button->setBounds(stackedItem(m_context.theme, BUTTON_WIDTH, BUTTON_HEIGHT, FIRST_BUTTON_Y,
                                      index++, BUTTON_GAP));
    }
}

void MainMenuScreen::continueGame() {
    if (!m_context.profiles.activateSlot(m_context.profiles.settings().lastSlot)) {
        return;
    }
    if (!m_context.resumeSnapshot()) {
        m_context.navigator.replaceWith(app::ScreenId::LevelSelect);
    }
}

void MainMenuScreen::handleAction(const Widget* fired) {
    if (fired == &m_continue) {
        continueGame();
    } else if (fired == &m_newGame) {
        m_context.navigator.replaceWith(app::ScreenId::NewGame);
    } else if (fired == &m_loadGame) {
        m_context.navigator.replaceWith(app::ScreenId::LoadGame);
    } else if (fired == &m_settings) {
        m_context.navigator.push(app::ScreenId::Settings);
    } else if (fired == &m_help) {
        m_context.navigator.push(app::ScreenId::Help);
    } else if (fired == &m_quit) {
        m_context.navigator.quit();
    }
}

void MainMenuScreen::update(float deltaSeconds, const app::InputState& input) {
    m_context.menuBackground.update(deltaSeconds);
    m_noticeSeconds = std::max(0.0f, m_noticeSeconds - deltaSeconds);
    if (!m_context.profiles.lastError().empty()) {
        m_notice = m_context.profiles.lastError(); // e.g. a snapshot that could not be resumed
        m_noticeSeconds = NOTICE_SECONDS;
        m_context.profiles.lastError().clear();
    }
    m_continue.setEnabled(canContinue());
    layout();
    handleAction(m_focus.update(input));
}

void MainMenuScreen::draw() {
    const render::Theme& theme = m_context.theme;
    drawMenuBackdrop(m_context);
    const float centerX = static_cast<float>(GetScreenWidth()) / 2.0f;
    render::drawGlow(Vector2{centerX, theme.px(TITLE_Y)}, theme.px(380.0f),
                     render::fade(palette::WALL_GLOW, 0.16f));
    theme.drawGlowText("MISSION ESCAPE", Vector2{centerX, theme.px(TITLE_Y)},
                       theme.px(render::textsize::TITLE), palette::TEXT, palette::ACCENT_GREEN);
    theme.drawTextCentered("twenty levels  |  four tiers  |  one way out",
                           Vector2{centerX, theme.px(TITLE_Y + 60.0f)},
                           theme.px(render::textsize::BODY), palette::ACCENT_GREEN);
    m_focus.draw(theme, m_context.time);
    if (canContinue()) {
        const core::SaveSlot& slot =
            m_context.profiles
                .slots()[static_cast<std::size_t>(m_context.profiles.settings().lastSlot - 1)];
        const Rectangle bounds = m_continue.bounds();
        const std::string detail = slot.data->playerName + "  -  level " +
                                   std::to_string(slot.data->progress.highestUnlockedLevel());
        theme.drawText(detail,
                       Vector2{bounds.x + bounds.width + theme.px(16.0f),
                               bounds.y + bounds.height / 2.0f - theme.px(9.0f)},
                       theme.px(render::textsize::SMALL), palette::TEXT_MUTED);
    }
    if (m_noticeSeconds > 0.0f) {
        const float alpha = std::min(1.0f, m_noticeSeconds);
        theme.drawTextCentered(
            m_notice, Vector2{centerX, static_cast<float>(GetScreenHeight()) - theme.px(56.0f)},
            theme.px(render::textsize::SMALL), render::fade(palette::WALL_GLOW, alpha));
    }
    drawFooterHint(theme, "ARROWS + ENTER or MOUSE to choose     ESC goes back in every menu");
}

} // namespace ui
