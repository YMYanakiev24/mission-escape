#include "ui/CaughtScreen.h"

#include "app/GameSession.h"
#include "core/Scoring.h"
#include "render/Palette.h"
#include "render/Primitives.h"

#include <raylib.h>

#include <algorithm>
#include <string>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float PANEL_WIDTH = 560.0f;
constexpr float PANEL_HEIGHT = 330.0f;
constexpr float BUTTON_WIDTH = 200.0f;
constexpr float BUTTON_HEIGHT = 50.0f;
constexpr float APPEAR_SECONDS = 0.3f;

} // namespace

CaughtScreen::CaughtScreen(app::AppContext& context) : m_context(context) {
    m_focus.setFeedback(&context.uiSounds);
    m_focus.add(m_retry);
    m_focus.add(m_levelSelect);
}

void CaughtScreen::onEnter() {
    m_elapsed = 0.0f;
    m_focus.focus(m_retry);
}

Rectangle CaughtScreen::panelBounds() const {
    const render::Theme& theme = m_context.theme;
    const float width = theme.px(PANEL_WIDTH);
    const float height = theme.px(PANEL_HEIGHT);
    return Rectangle{(static_cast<float>(GetScreenWidth()) - width) / 2.0f,
                     (static_cast<float>(GetScreenHeight()) - height) / 2.0f, width, height};
}

void CaughtScreen::layout() {
    const render::Theme& theme = m_context.theme;
    const Rectangle panel = panelBounds();
    const float width = theme.px(BUTTON_WIDTH);
    const float height = theme.px(BUTTON_HEIGHT);
    const float y = panel.y + panel.height - height - theme.px(28.0f);
    const float centerX = panel.x + panel.width / 2.0f;
    m_retry.setBounds(Rectangle{centerX - width - theme.px(10.0f), y, width, height});
    m_levelSelect.setBounds(Rectangle{centerX + theme.px(10.0f), y, width, height});
}

void CaughtScreen::update(float deltaSeconds, const app::InputState& input) {
    m_elapsed += deltaSeconds;
    if (m_context.session) {
        m_context.session->refreshView();
    }
    layout();
    const Widget* fired = m_focus.update(input);
    if (fired == &m_retry) {
        m_context.retryLevel();
    } else if (fired == &m_levelSelect || input.back) {
        m_context.endSession();
        m_context.navigator.replaceWith(app::ScreenId::LevelSelect);
    }
}

void CaughtScreen::draw() {
    const render::Theme& theme = m_context.theme;
    const float appear = render::easeOutCubic(std::min(1.0f, m_elapsed / APPEAR_SECONDS));
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  render::fade(palette::BG_DEEP, 0.8f * appear));
    const Rectangle panel = panelBounds();
    render::drawPanel(panel, theme.px(18.0f), render::fade(palette::PANEL, 0.97f),
                      render::fade(palette::DANGER, 0.8f), theme.px(2.0f));
    const float centerX = panel.x + panel.width / 2.0f;
    const float scale = 0.8f + 0.2f * render::easeOutBack(std::min(1.0f, m_elapsed / 0.5f));
    theme.drawGlowText("CAUGHT!", Vector2{centerX, panel.y + theme.px(70.0f)},
                       theme.px(render::textsize::TITLE * 0.8f) * scale, palette::TEXT,
                       palette::DANGER);
    if (m_context.session) {
        const core::Level& level = m_context.session->level();
        theme.drawTextCentered("The Hunter found you on level " +
                                   std::to_string(level.levelNumber()) + " after " +
                                   core::formatDuration(level.elapsedSeconds()) + ".",
                               Vector2{centerX, panel.y + theme.px(140.0f)},
                               theme.px(render::textsize::BODY), palette::TEXT);
    }
    theme.drawTextCentered("No stars this time. Retry the same maze or pick another level.",
                           Vector2{centerX, panel.y + theme.px(176.0f)},
                           theme.px(render::textsize::SMALL), palette::TEXT_MUTED);
    m_focus.draw(theme, m_context.time);
}

} // namespace ui
