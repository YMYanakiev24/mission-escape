#include "ui/LevelCompleteScreen.h"

#include "app/GameSession.h"

#include "core/Config.h"
#include "core/Scoring.h"
#include "render/Hud.h"
#include "render/Palette.h"
#include "render/Primitives.h"
#include "render/SkinRenderer.h"

#include <raylib.h>

#include <algorithm>
#include <string>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float PANEL_WIDTH = 600.0f;
constexpr float PANEL_HEIGHT = 450.0f;
constexpr float BUTTON_WIDTH = 168.0f;
constexpr float BUTTON_HEIGHT = 50.0f;
constexpr float BUTTON_GAP = 16.0f;
constexpr float STAR_RADIUS = 36.0f;
constexpr float STAR_SPACING = 96.0f;
constexpr float FIRST_STAR_DELAY = 0.45f;
constexpr float STAR_INTERVAL = 0.38f;
constexpr float STAR_POP_SECONDS = 0.45f;
constexpr float STAR_PITCH_STEP = 0.12f;

} // namespace

LevelCompleteScreen::LevelCompleteScreen(app::AppContext& context) : m_context(context) {
    m_focus.setFeedback(&context.uiSounds);
    m_focus.add(m_next);
    m_focus.add(m_retry);
    m_focus.add(m_levelSelect);
}

void LevelCompleteScreen::onEnter() {
    m_elapsed = 0.0f;
    m_starsAnnounced = 0;
    const bool lastLevel = m_context.lastOutcome.level >= core::LEVEL_COUNT;
    m_next.setLabel(lastLevel ? "Finish" : "Next Level");
    m_focus.focus(m_next);
}

Rectangle LevelCompleteScreen::panelBounds() const {
    const render::Theme& theme = m_context.theme;
    const float width = theme.px(PANEL_WIDTH);
    const float height = theme.px(PANEL_HEIGHT);
    return Rectangle{(static_cast<float>(GetScreenWidth()) - width) / 2.0f,
                     (static_cast<float>(GetScreenHeight()) - height) / 2.0f, width, height};
}

void LevelCompleteScreen::layout() {
    const render::Theme& theme = m_context.theme;
    const Rectangle panel = panelBounds();
    const float width = theme.px(BUTTON_WIDTH);
    const float height = theme.px(BUTTON_HEIGHT);
    const float gap = theme.px(BUTTON_GAP);
    const float y = panel.y + panel.height - height - theme.px(28.0f);
    float x = panel.x + (panel.width - 3.0f * width - 2.0f * gap) / 2.0f;
    for (Button* button : {&m_next, &m_retry, &m_levelSelect}) {
        button->setBounds(Rectangle{x, y, width, height});
        x += width + gap;
    }
}

void LevelCompleteScreen::update(float deltaSeconds, const app::InputState& input) {
    m_elapsed += deltaSeconds;
    playStarDings();
    if (m_context.session) {
        m_context.session->refreshView();
    }
    layout();
    const Widget* fired = m_focus.update(input);
    const int level = m_context.lastOutcome.level;
    if (fired == &m_next) {
        if (level >= core::LEVEL_COUNT) {
            m_context.endSession();
            m_context.navigator.replaceWith(app::ScreenId::GameComplete);
        } else {
            m_context.startLevel(level + 1);
        }
    } else if (fired == &m_retry) {
        m_context.retryLevel();
    } else if (fired == &m_levelSelect || input.back) {
        m_context.endSession();
        m_context.navigator.replaceWith(app::ScreenId::LevelSelect);
    }
}

void LevelCompleteScreen::playStarDings() {
    // One rising "ding" per star, in sync with the star pop-in animation.
    const int earned = m_context.lastOutcome.stars;
    while (m_starsAnnounced < earned &&
           m_elapsed >= FIRST_STAR_DELAY + static_cast<float>(m_starsAnnounced) * STAR_INTERVAL) {
        m_context.audio.play(audio::SoundId::StarDing, 1.0f,
                             1.0f + STAR_PITCH_STEP * static_cast<float>(m_starsAnnounced));
        ++m_starsAnnounced;
    }
}

void LevelCompleteScreen::drawStars(Rectangle panel) const {
    const render::Theme& theme = m_context.theme;
    const float centerX = panel.x + panel.width / 2.0f;
    const float y = panel.y + theme.px(150.0f);
    for (int index = 0; index < core::MAX_STARS; ++index) {
        const Vector2 center{centerX + static_cast<float>(index - 1) * theme.px(STAR_SPACING), y};
        const float radius = theme.px(STAR_RADIUS);
        render::drawStar(center, radius, 0.0f, render::fade(palette::BG_DEEP, 0.9f));
        render::drawStarOutline(center, radius, theme.px(2.0f),
                                render::fade(palette::TEXT_MUTED, 0.4f));
        if (index >= m_context.lastOutcome.stars) {
            continue;
        }
        const float appearAt = FIRST_STAR_DELAY + static_cast<float>(index) * STAR_INTERVAL;
        const float progress = std::clamp((m_elapsed - appearAt) / STAR_POP_SECONDS, 0.0f, 1.0f);
        if (progress <= 0.0f) {
            continue;
        }
        const float scale = render::easeOutBack(progress);
        render::drawGlow(center, radius * 2.0f * scale,
                         render::fade(palette::ACCENT_GREEN, 0.45f * progress));
        render::drawStar(center, radius * scale, 0.0f, palette::ACCENT_GREEN);
        render::drawStar(center, radius * scale * 0.45f, 0.0f, render::fade(palette::TEXT, 0.8f));
    }
}

void LevelCompleteScreen::drawDetails(Rectangle panel) const {
    const render::Theme& theme = m_context.theme;
    const app::LevelOutcome& outcome = m_context.lastOutcome;
    const float centerX = panel.x + panel.width / 2.0f;
    float y = panel.y + theme.px(222.0f);
    theme.drawTextCentered("Time  " + core::formatDuration(outcome.seconds), Vector2{centerX, y},
                           theme.px(render::textsize::LARGE), palette::TEXT);
    y += theme.px(38.0f);
    const std::string thresholds =
        "3 stars under " + core::formatDuration(core::starTimeLimit(3, outcome.parSeconds)) +
        "   |   2 stars under " + core::formatDuration(core::starTimeLimit(2, outcome.parSeconds));
    theme.drawTextCentered(thresholds, Vector2{centerX, y}, theme.px(render::textsize::SMALL),
                           palette::TEXT_MUTED);
    y += theme.px(32.0f);
    if (outcome.newRecord) {
        const float glow = 0.6f + 0.4f * render::pulse(m_context.time, 1.5f);
        theme.drawTextCentered("NEW RECORD!", Vector2{centerX, y}, theme.px(render::textsize::BODY),
                               render::fade(palette::ACCENT_GREEN, glow));
    } else if (outcome.bestStars > 0) {
        theme.drawTextCentered("Best: " + std::to_string(outcome.bestStars) + " stars in " +
                                   core::formatDuration(outcome.bestSeconds),
                               Vector2{centerX, y}, theme.px(render::textsize::BODY),
                               palette::TEXT_MUTED);
    }
    if (outcome.unlockedSkin) {
        y += theme.px(36.0f);
        const std::string text =
            "New skin unlocked: " + std::string(core::skinName(*outcome.unlockedSkin)) + "!";
        const float textWidth = theme.measure(text, theme.px(render::textsize::BODY)).x;
        const float radius = theme.px(13.0f);
        render::drawSkin(*outcome.unlockedSkin,
                         Vector2{centerX - textWidth / 2.0f - radius * 2.0f, y}, radius,
                         m_context.time, m_context.time);
        theme.drawTextCentered(text, Vector2{centerX + radius, y}, theme.px(render::textsize::BODY),
                               palette::ACCENT_GREEN);
    }
}

void LevelCompleteScreen::draw() {
    const render::Theme& theme = m_context.theme;
    const float appear = render::easeOutCubic(std::min(1.0f, m_elapsed / 0.35f));
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  render::fade(palette::BG_DEEP, 0.78f * appear));
    const Rectangle panel = panelBounds();
    render::drawPanel(panel, theme.px(18.0f), render::fade(palette::PANEL, 0.97f),
                      palette::ACCENT_GREEN_DIM, theme.px(2.0f));
    theme.drawGlowText("LEVEL " + std::to_string(m_context.lastOutcome.level) + " COMPLETE",
                       Vector2{panel.x + panel.width / 2.0f, panel.y + theme.px(56.0f)},
                       theme.px(render::textsize::HEADING), palette::TEXT, palette::ACCENT_GREEN);
    drawStars(panel);
    drawDetails(panel);
    m_focus.draw(theme, m_context.time);
}

} // namespace ui
