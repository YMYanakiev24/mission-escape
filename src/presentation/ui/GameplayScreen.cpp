#include "ui/GameplayScreen.h"

#include "app/GameSession.h"
#include "core/LevelRules.h"
#include "render/Palette.h"
#include "render/Primitives.h"
#include "render/TierStyle.h"

#include <raylib.h>

#include <array>
#include <string>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float RESULT_DELAY_SECONDS = 0.9f;
constexpr float PAUSE_PANEL_WIDTH = 440.0f;
constexpr float PAUSE_BUTTON_HEIGHT = 50.0f;
constexpr float PAUSE_BUTTON_GAP = 12.0f;
constexpr float PAUSE_HEADER_HEIGHT = 110.0f;
constexpr float PAUSE_PADDING = 28.0f;
constexpr int PAUSE_BUTTON_COUNT = 6;
constexpr float INTRO_WIDTH = 640.0f;
constexpr float INTRO_HEIGHT = 340.0f;

} // namespace

GameplayScreen::GameplayScreen(app::AppContext& context) : m_context(context) {
    m_pauseFocus.setFeedback(&context.uiSounds);
    m_confirm.setFeedback(&context.uiSounds);
    for (Button* button :
         {&m_resume, &m_restart, &m_settings, &m_help, &m_saveAndExit, &m_quitLevel}) {
        m_pauseFocus.add(*button);
    }
}

void GameplayScreen::onEnter() {
    m_paused = false;
    m_resultShown = false;
    m_confirm.close();
    const core::SaveData* profile = m_context.profiles.activeProfile();
    m_showTierIntro = m_context.session && profile != nullptr &&
                      !profile->progress.hasSeenTierIntro(m_context.session->level().spec().tier);
}

void GameplayScreen::onResume() {
    // Coming back from Settings or Help: stay in the pause menu.
    m_paused = !m_resultShown;
}

void GameplayScreen::update(float deltaSeconds, const app::InputState& input) {
    if (!m_context.session) {
        m_context.navigator.replaceWith(app::ScreenId::MainMenu);
        return;
    }
    app::GameSession& session = *m_context.session;
    if (m_showTierIntro) {
        updateTierIntro(input);
        session.refreshView();
        return;
    }
    if (m_confirm.isOpen()) {
        handleConfirmDialog(input);
        session.refreshView();
        return;
    }
    if (m_paused) {
        updatePause(input);
        session.refreshView();
        return;
    }
    const bool focusLost = !IsWindowFocused() && !m_context.options.screenshotTour;
    const bool canPause = !session.level().isFinished();
    if (canPause && (app::isActionPressed(app::GameAction::Pause) || focusLost)) {
        openPause();
        session.refreshView();
        return;
    }
    // Quick restart for chasing better times: same maze, fresh timer, no menu in between.
    if (canPause && session.level().timerStarted() &&
        app::isActionPressed(app::GameAction::Restart)) {
        m_context.restartLevel();
        m_context.uiSounds.activated();
        session.refreshView();
        return;
    }
    session.update(deltaSeconds, input);
    checkFinished();
}

void GameplayScreen::checkFinished() {
    const app::GameSession& session = *m_context.session;
    if (m_resultShown || session.secondsSinceFinished() < RESULT_DELAY_SECONDS) {
        return;
    }
    const core::Level& level = session.level();
    if (level.state() == core::LevelState::Won) {
        m_context.recordLevelResult(level);
        m_resultShown = true;
        m_context.navigator.push(app::ScreenId::LevelComplete);
    } else if (level.state() == core::LevelState::Caught) {
        // Caught: 0 stars and records untouched. A resumed save of this attempt is gone too,
        // otherwise "Continue" would undo the catch.
        m_context.profiles.discardSavedAttempt();
        m_resultShown = true;
        m_context.navigator.push(app::ScreenId::Caught);
    }
}

void GameplayScreen::updateTierIntro(const app::InputState& input) {
    if (!(input.confirm || input.back || input.mousePressed)) {
        return;
    }
    m_showTierIntro = false;
    if (core::SaveData* profile = m_context.profiles.activeProfile()) {
        profile->progress.markTierIntroSeen(m_context.session->level().spec().tier);
        m_context.profiles.saveActiveProfile();
    }
}

void GameplayScreen::drawTierIntro() const {
    const render::Theme& theme = m_context.theme;
    const core::Tier tier = m_context.session->level().spec().tier;
    const Color accent = render::tierStyle(tier).accent;
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), render::fade(palette::BG_DEEP, 0.82f));
    const float width = theme.px(INTRO_WIDTH);
    const float height = theme.px(INTRO_HEIGHT);
    const Rectangle panel{(static_cast<float>(GetScreenWidth()) - width) / 2.0f,
                          (static_cast<float>(GetScreenHeight()) - height) / 2.0f, width, height};
    render::drawPanel(panel, theme.px(18.0f), render::fade(palette::PANEL, 0.98f), accent,
                      theme.px(2.5f));
    const float centerX = panel.x + panel.width / 2.0f;
    const int first = core::firstLevelOfTier(tier);
    theme.drawTextCentered("TIER " + std::to_string(core::tierNumber(tier)) + "   |   LEVELS " +
                               std::to_string(first) + "-" +
                               std::to_string(first + core::LEVELS_PER_TIER - 1),
                           Vector2{centerX, panel.y + theme.px(44.0f)},
                           theme.px(render::textsize::SMALL), palette::TEXT_MUTED);
    theme.drawGlowText(std::string(core::tierName(tier)),
                       Vector2{centerX, panel.y + theme.px(100.0f)},
                       theme.px(render::textsize::TITLE * 0.85f), palette::TEXT, accent);
    const float padding = theme.px(40.0f);
    theme.drawWrapped(
        core::tierDescription(tier), Vector2{panel.x + padding, panel.y + theme.px(160.0f)},
        panel.width - 2.0f * padding, theme.px(render::textsize::BODY), palette::TEXT);
    const float blink = 0.5f + 0.5f * render::pulse(m_context.time, 1.0f);
    theme.drawTextCentered(app::isUsingGamepad() ? "press A to begin" : "press ENTER to begin",
                           Vector2{centerX, panel.y + panel.height - theme.px(36.0f)},
                           theme.px(render::textsize::SMALL),
                           render::fade(accent, 0.5f + 0.5f * blink));
}

void GameplayScreen::openPause() {
    m_paused = true;
    m_pauseFocus.focus(m_resume);
}

void GameplayScreen::updatePause(const app::InputState& input) {
    layoutPause();
    if (input.back || app::isActionPressed(app::GameAction::Pause)) {
        m_paused = false;
        return;
    }
    handlePauseAction(m_pauseFocus.update(input));
}

void GameplayScreen::handlePauseAction(const Widget* fired) {
    if (fired == &m_resume) {
        m_paused = false;
    } else if (fired == &m_restart) {
        m_context.restartLevel();
        m_paused = false;
    } else if (fired == &m_settings) {
        m_context.navigator.push(app::ScreenId::Settings);
    } else if (fired == &m_help) {
        m_context.navigator.push(app::ScreenId::Help);
    } else if (fired == &m_saveAndExit) {
        m_context.saveAndExitSession();
    } else if (fired == &m_quitLevel) {
        m_confirm.open("Quit level?",
                       "You will get 0 stars for this attempt. Your previous best "
                       "result for this level is kept.",
                       "Quit Level", "Keep Playing");
    }
}

void GameplayScreen::handleConfirmDialog(const app::InputState& input) {
    const DialogResult result = m_confirm.update(m_context.theme, input);
    if (result == DialogResult::Confirmed) {
        // A quit scores 0 stars, leaves every record untouched and abandons any resumed save.
        m_context.profiles.discardSavedAttempt();
        m_context.endSession();
        m_context.navigator.replaceWith(app::ScreenId::LevelSelect);
    }
}

void GameplayScreen::layoutPause() {
    const render::Theme& theme = m_context.theme;
    const float width = theme.px(PAUSE_PANEL_WIDTH);
    const float buttonHeight = theme.px(PAUSE_BUTTON_HEIGHT);
    const float gap = theme.px(PAUSE_BUTTON_GAP);
    const float padding = theme.px(PAUSE_PADDING);
    const float panelHeight = theme.px(PAUSE_HEADER_HEIGHT) + padding +
                              static_cast<float>(PAUSE_BUTTON_COUNT) * (buttonHeight + gap);
    const float x = (static_cast<float>(GetScreenWidth()) - width) / 2.0f;
    float y = (static_cast<float>(GetScreenHeight()) - panelHeight) / 2.0f +
              theme.px(PAUSE_HEADER_HEIGHT);
    for (Button* button :
         {&m_resume, &m_restart, &m_settings, &m_help, &m_saveAndExit, &m_quitLevel}) {
        button->setBounds(Rectangle{x + padding, y, width - 2.0f * padding, buttonHeight});
        y += buttonHeight + gap;
    }
}

void GameplayScreen::drawPause() const {
    const render::Theme& theme = m_context.theme;
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), render::fade(palette::BG_DEEP, 0.72f));
    const Rectangle first = m_resume.bounds();
    const Rectangle last = m_quitLevel.bounds();
    const float padding = theme.px(PAUSE_PADDING);
    const Rectangle panel{
        first.x - padding, first.y - theme.px(PAUSE_HEADER_HEIGHT), first.width + 2.0f * padding,
        last.y + last.height + padding - (first.y - theme.px(PAUSE_HEADER_HEIGHT))};
    render::drawPanel(panel, theme.px(16.0f), render::fade(palette::PANEL, 0.97f), palette::WALL,
                      theme.px(2.0f));
    const float centerX = panel.x + panel.width / 2.0f;
    theme.drawGlowText("PAUSED", Vector2{centerX, panel.y + theme.px(42.0f)},
                       theme.px(render::textsize::HEADING), palette::TEXT, palette::WALL_GLOW);
    const core::Level& level = m_context.session->level();
    const std::string subtitle = "Level " + std::to_string(level.levelNumber()) + "  |  " +
                                 std::string(core::tierName(level.spec().tier)) +
                                 "  |  timer frozen";
    theme.drawTextCentered(subtitle, Vector2{centerX, panel.y + theme.px(80.0f)},
                           theme.px(render::textsize::SMALL), palette::TEXT_MUTED);
    m_pauseFocus.draw(theme, m_context.time);
}

void GameplayScreen::draw() {
    if (!m_context.session) {
        ClearBackground(palette::BG_DEEP);
        return;
    }
    m_context.session->draw();
    if (m_paused) {
        drawPause();
    }
    if (m_showTierIntro) {
        drawTierIntro();
    }
    m_confirm.draw(m_context.theme, m_context.time);
}

} // namespace ui
