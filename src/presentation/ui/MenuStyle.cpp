#include "ui/MenuStyle.h"

#include "render/Palette.h"
#include "render/Primitives.h"

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float TITLE_Y = 70.0f;
constexpr float SUBTITLE_OFFSET = 48.0f;
constexpr float FOOTER_OFFSET = 26.0f;
constexpr float PANEL_TOP = 150.0f;
constexpr float PANEL_BOTTOM_MARGIN = 120.0f;
constexpr float PANEL_WIDTH = 920.0f;

} // namespace

void drawMenuBackdrop(const app::AppContext& context) {
    context.menuBackground.draw(context.time);
}

void drawScreenTitle(const render::Theme& theme, std::string_view title,
                     std::string_view subtitle) {
    const float centerX = static_cast<float>(GetScreenWidth()) / 2.0f;
    theme.drawGlowText(title, Vector2{centerX, theme.px(TITLE_Y)},
                       theme.px(render::textsize::HEADING), palette::TEXT, palette::WALL_GLOW);
    if (!subtitle.empty()) {
        theme.drawTextCentered(subtitle, Vector2{centerX, theme.px(TITLE_Y + SUBTITLE_OFFSET)},
                               theme.px(render::textsize::BODY), palette::TEXT_MUTED);
    }
}

void drawFooterHint(const render::Theme& theme, std::string_view hint) {
    theme.drawTextCentered(hint,
                           Vector2{static_cast<float>(GetScreenWidth()) / 2.0f,
                                   static_cast<float>(GetScreenHeight()) - theme.px(FOOTER_OFFSET)},
                           theme.px(render::textsize::TINY), palette::TEXT_MUTED);
}

Rectangle stackedItem(const render::Theme& theme, float width, float height, float top, int index,
                      float gap) {
    const float scaledWidth = theme.px(width);
    const float scaledHeight = theme.px(height);
    return Rectangle{(static_cast<float>(GetScreenWidth()) - scaledWidth) / 2.0f,
                     theme.px(top) + static_cast<float>(index) * (scaledHeight + theme.px(gap)),
                     scaledWidth, scaledHeight};
}

Rectangle contentPanel(const render::Theme& theme) {
    const float width = theme.px(PANEL_WIDTH);
    const float top = theme.px(PANEL_TOP);
    return Rectangle{(static_cast<float>(GetScreenWidth()) - width) / 2.0f, top, width,
                     static_cast<float>(GetScreenHeight()) - top - theme.px(PANEL_BOTTOM_MARGIN)};
}

void drawContentPanel(const render::Theme& theme, Rectangle bounds) {
    render::drawPanel(bounds, theme.px(16.0f), render::fade(palette::PANEL, 0.97f),
                      render::fade(palette::WALL, 0.8f), theme.px(1.5f));
}

} // namespace ui
