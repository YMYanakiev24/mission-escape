#include "ui/SplashScreen.h"

#include "render/Palette.h"
#include "render/Primitives.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float SPLASH_SECONDS = 3.0f;
constexpr float MIN_SKIP_SECONDS = 0.4f;
constexpr float FADE_IN_SECONDS = 1.1f;
constexpr int GRID_LINES = 18;

} // namespace

void SplashScreen::update(float deltaSeconds, const app::InputState& input) {
    m_elapsed += deltaSeconds;
    const bool skipped = input.anyPressed && m_elapsed > MIN_SKIP_SECONDS;
    if (skipped || m_elapsed >= SPLASH_SECONDS) {
        m_context.navigator.replaceWith(app::ScreenId::MainMenu);
    }
}

void SplashScreen::draw() {
    const render::Theme& theme = m_context.theme;
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    ClearBackground(palette::BG_DEEP);

    // A receding neon grid that slowly scrolls towards the viewer.
    const float scroll = std::fmod(m_elapsed * 0.35f, 1.0f);
    for (int line = 0; line < GRID_LINES; ++line) {
        const float depth = (static_cast<float>(line) + scroll) / static_cast<float>(GRID_LINES);
        const float y = height * 0.62f + depth * depth * height * 0.38f;
        DrawLineEx(Vector2{0.0f, y}, Vector2{width, y}, 1.0f,
                   render::fade(palette::WALL, 0.15f + 0.5f * depth));
    }
    for (int column = -GRID_LINES; column <= GRID_LINES; ++column) {
        const float spread = static_cast<float>(column) / static_cast<float>(GRID_LINES);
        DrawLineEx(Vector2{width / 2.0f + spread * width * 0.08f, height * 0.62f},
                   Vector2{width / 2.0f + spread * width * 1.4f, height}, 1.0f,
                   render::fade(palette::WALL, 0.35f));
    }

    const float appear = render::easeOutCubic(std::min(1.0f, m_elapsed / FADE_IN_SECONDS));
    const Vector2 center{width / 2.0f, height * 0.40f};
    render::drawGlow(center, theme.px(420.0f) * appear,
                     render::fade(palette::WALL_GLOW, 0.20f * appear));
    theme.drawGlowText("MISSION ESCAPE", center, theme.px(render::textsize::TITLE),
                       render::fade(palette::TEXT, appear),
                       render::fade(palette::ACCENT_GREEN, appear));
    theme.drawTextCentered(
        "find the way out of the light", Vector2{center.x, center.y + theme.px(62.0f)},
        theme.px(render::textsize::BODY), render::fade(palette::ACCENT_GREEN, appear));
    const float blink = 0.4f + 0.6f * render::pulse(m_elapsed, 0.8f);
    theme.drawTextCentered("press any key", Vector2{center.x, height * 0.86f},
                           theme.px(render::textsize::SMALL),
                           render::fade(palette::TEXT_MUTED, blink * appear));
}

} // namespace ui
