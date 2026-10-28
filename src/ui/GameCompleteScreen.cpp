#include "ui/GameCompleteScreen.h"

#include "core/Config.h"
#include "render/Palette.h"
#include "render/Primitives.h"
#include "ui/MenuStyle.h"

#include <raylib.h>

#include <cmath>
#include <string>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr int ORBIT_STARS = 12;
constexpr float ORBIT_SPEED = 0.35f;
constexpr float TITLE_Y = 0.30f;

} // namespace

GameCompleteScreen::GameCompleteScreen(app::AppContext& context) : m_context(context) {
    m_focus.setFeedback(&context.uiSounds);
    m_focus.add(m_levelSelect);
    m_focus.add(m_mainMenu);
}

void GameCompleteScreen::onEnter() {
    m_elapsed = 0.0f;
    m_focus.focus(m_levelSelect);
}

void GameCompleteScreen::layout() {
    const render::Theme& theme = m_context.theme;
    const float width = theme.px(240.0f);
    const float height = theme.px(52.0f);
    const float centerX = static_cast<float>(GetScreenWidth()) / 2.0f;
    const float y = static_cast<float>(GetScreenHeight()) * 0.74f;
    m_levelSelect.setBounds(Rectangle{centerX - width - theme.px(10.0f), y, width, height});
    m_mainMenu.setBounds(Rectangle{centerX + theme.px(10.0f), y, width, height});
}

void GameCompleteScreen::update(float deltaSeconds, const app::InputState& input) {
    m_elapsed += deltaSeconds;
    m_context.menuBackground.update(deltaSeconds);
    layout();
    const Widget* fired = m_focus.update(input);
    if (fired == &m_levelSelect) {
        m_context.navigator.replaceWith(app::ScreenId::LevelSelect);
    } else if (fired == &m_mainMenu || input.back) {
        m_context.navigator.replaceWith(app::ScreenId::MainMenu);
    }
}

void GameCompleteScreen::draw() {
    const render::Theme& theme = m_context.theme;
    drawMenuBackdrop(m_context);
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    const Vector2 center{width / 2.0f, height * TITLE_Y};
    render::drawGlow(center, theme.px(420.0f), render::fade(palette::ACCENT_GREEN, 0.22f));
    for (int index = 0; index < ORBIT_STARS; ++index) {
        const float angle = m_elapsed * ORBIT_SPEED +
                            static_cast<float>(index) * 2.0f * PI / static_cast<float>(ORBIT_STARS);
        const Vector2 position{center.x + std::cos(angle) * theme.px(330.0f),
                               center.y + std::sin(angle) * theme.px(120.0f)};
        render::drawStar(
            position,
            theme.px(9.0f + 3.0f * render::pulse(m_elapsed + static_cast<float>(index), 0.7f)),
            m_elapsed * 40.0f, render::fade(palette::ACCENT_GREEN, 0.85f));
    }
    theme.drawGlowText("YOU ESCAPED THE LABYRINTH", center,
                       theme.px(render::textsize::HEADING * 1.2f), palette::TEXT,
                       palette::ACCENT_GREEN);
    const core::SaveData* profile = m_context.activeProfile();
    if (profile != nullptr) {
        const std::string summary = profile->playerName + " cleared all " +
                                    std::to_string(core::LEVEL_COUNT) + " levels with " +
                                    std::to_string(profile->progress.totalStars()) + " of " +
                                    std::to_string(core::LEVEL_COUNT * core::MAX_STARS) + " stars.";
        theme.drawTextCentered(summary, Vector2{center.x, center.y + theme.px(70.0f)},
                               theme.px(render::textsize::LARGE), palette::TEXT);
    }
    theme.drawTextCentered("Every skin is yours. Replay any level to chase three stars.",
                           Vector2{center.x, center.y + theme.px(112.0f)},
                           theme.px(render::textsize::BODY), palette::TEXT_MUTED);
    m_focus.draw(theme, m_context.time);
}

} // namespace ui
