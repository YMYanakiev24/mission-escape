#include "ui/HelpScreen.h"

#include "core/Config.h"
#include "core/LevelRules.h"
#include "core/Perks.h"
#include "render/Palette.h"
#include "render/PerkIcons.h"
#include "render/Primitives.h"
#include "render/TierStyle.h"
#include "ui/MenuStyle.h"

#include <raylib.h>

#include <array>
#include <string>
#include <utility>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float TAB_ROW_HEIGHT = 46.0f;
constexpr float PANEL_PADDING = 28.0f;
constexpr float ROW_HEIGHT = 31.0f;
constexpr float KEY_COLUMN = 190.0f;

using KeyLine = std::pair<const char*, const char*>;

constexpr std::array<KeyLine, 6> CONTROLS_2D = {
    KeyLine{"Arrows / WASD", "Move one cell (hold to keep walking)"},
    KeyLine{"Shift", "Use a Magic Berry: phase through walls"},
    KeyLine{"T", "Toggle the ASCII text view"},
    KeyLine{"Mouse", "Hover a cell to see what it is"},
    KeyLine{"Esc / P", "Pause (the timer stops)"},
    KeyLine{"Text view", "# wall, % secret door, @ you, E exit"}};

constexpr std::array<KeyLine, 6> CONTROLS_3D = {KeyLine{"W / Up", "Step forward"},
                                                KeyLine{"S / Down", "Step back"},
                                                KeyLine{"A D / Left Right", "Turn 90 degrees"},
                                                KeyLine{"Q / E", "Strafe left / right"},
                                                KeyLine{"M", "Small / large minimap"},
                                                KeyLine{"Shift", "Use a Magic Berry"}};

/// Draws "key  description" rows starting at @p origin; returns the y below the last row.
float drawKeyLines(const render::Theme& theme, Vector2 origin,
                   const std::array<KeyLine, 6>& lines) {
    float y = origin.y;
    for (const KeyLine& line : lines) {
        theme.drawText(line.first, Vector2{origin.x, y}, theme.px(render::textsize::SMALL),
                       palette::ACCENT_GREEN);
        theme.drawText(line.second, Vector2{origin.x + theme.px(KEY_COLUMN), y},
                       theme.px(render::textsize::SMALL), palette::TEXT);
        y += theme.px(ROW_HEIGHT);
    }
    return y;
}

} // namespace

HelpScreen::HelpScreen(app::AppContext& context) : m_context(context) {
    m_focus.setFeedback(&context.uiSounds);
    m_tabs.addTo(m_focus);
    m_focus.add(m_back);
}

void HelpScreen::onEnter() {
    m_tabs.setActive(0);
    m_focus.focus(m_tabs.button(0));
}

void HelpScreen::layout() {
    const render::Theme& theme = m_context.theme;
    const Rectangle panel = contentPanel(theme);
    m_tabs.layout(Rectangle{panel.x, panel.y - theme.px(TAB_ROW_HEIGHT + 10.0f), panel.width,
                            theme.px(TAB_ROW_HEIGHT)},
                  theme.px(10.0f));
    const float width = theme.px(220.0f);
    m_back.setBounds(Rectangle{(static_cast<float>(GetScreenWidth()) - width) / 2.0f,
                               panel.y + panel.height + theme.px(18.0f), width, theme.px(48.0f)});
}

void HelpScreen::update(float deltaSeconds, const app::InputState& input) {
    m_context.menuBackground.update(deltaSeconds);
    layout();
    const Widget* fired = m_focus.update(input);
    m_tabs.update(m_focus, fired);
    if (fired == &m_back || input.back) {
        m_context.navigator.pop();
    }
}

void HelpScreen::drawControls(Rectangle area) const {
    const render::Theme& theme = m_context.theme;
    const float columnWidth = area.width / 2.0f;
    theme.drawText("2D CLASSIC", Vector2{area.x, area.y}, theme.px(render::textsize::BODY),
                   palette::TEXT_MUTED);
    drawKeyLines(theme, Vector2{area.x, area.y + theme.px(40.0f)}, CONTROLS_2D);
    theme.drawText("3D FIRST-PERSON", Vector2{area.x + columnWidth, area.y},
                   theme.px(render::textsize::BODY), palette::TEXT_MUTED);
    drawKeyLines(theme, Vector2{area.x + columnWidth, area.y + theme.px(40.0f)}, CONTROLS_3D);
    theme.drawWrapped("Menus: arrow keys move the highlight, Enter selects, Esc goes back. "
                      "Everything also works with the mouse.",
                      Vector2{area.x, area.y + theme.px(260.0f)}, area.width,
                      theme.px(render::textsize::SMALL), palette::TEXT_MUTED);
}

void HelpScreen::drawPerks(Rectangle area) const {
    const render::Theme& theme = m_context.theme;
    const float rowHeight = theme.px(62.0f);
    float y = area.y;
    for (const core::PerkType type : core::ALL_PERK_TYPES) {
        const float iconRadius = theme.px(17.0f);
        render::drawPerkIcon(type, Vector2{area.x + iconRadius, y + iconRadius}, iconRadius,
                             m_context.time);
        const float textX = area.x + theme.px(56.0f);
        theme.drawText(std::string(core::perkName(type)), Vector2{textX, y - theme.px(2.0f)},
                       theme.px(render::textsize::BODY), render::perkColor(type));
        theme.drawText("(" + std::string(core::perkTierText(type)) + ")",
                       Vector2{textX + theme.px(190.0f), y + theme.px(2.0f)},
                       theme.px(render::textsize::TINY), palette::TEXT_MUTED);
        theme.drawText(std::string(core::perkDescription(type)),
                       Vector2{textX, y + theme.px(26.0f)}, theme.px(render::textsize::SMALL),
                       palette::TEXT);
        y += rowHeight;
    }
}

void HelpScreen::drawTiers(Rectangle area) const {
    const render::Theme& theme = m_context.theme;
    const float rowHeight = (area.height - theme.px(4.0f)) / static_cast<float>(core::TIER_COUNT);
    for (int index = 0; index < core::TIER_COUNT; ++index) {
        const core::Tier tier = static_cast<core::Tier>(index);
        const float y = area.y + static_cast<float>(index) * rowHeight;
        const int first = core::firstLevelOfTier(tier);
        const std::string heading = std::string(core::tierName(tier)) + "   levels " +
                                    std::to_string(first) + "-" +
                                    std::to_string(first + core::LEVELS_PER_TIER - 1);
        theme.drawText(heading, Vector2{area.x, y}, theme.px(render::textsize::BODY),
                       render::tierStyle(tier).accent);
        theme.drawWrapped(core::tierDescription(tier), Vector2{area.x, y + theme.px(28.0f)},
                          area.width, theme.px(render::textsize::SMALL), palette::TEXT);
    }
}

void HelpScreen::drawStars(Rectangle area) const {
    const render::Theme& theme = m_context.theme;
    const std::array<const char*, 7> lines = {
        "The timer starts with your first move and stops while paused or frozen by an Hourglass.",
        "Par time = shortest path length x 0.30 s, x1.5 in Fog levels, x0.85 in Labyrinth "
        "levels, x1.75 in 3D mode.",
        "3 stars: finish within 2.0 x par.     2 stars: within 3.5 x par.     1 star: any finish.",
        "Quitting a level gives 0 stars and never overwrites your previous best.",
        "Your best stars and best time are saved for every level.",
        "Skins unlock when you complete levels 5, 10, 15 and 20 (with at least one star).",
        "The HUD always shows the best rating still reachable and its time limit."};
    float y = area.y;
    for (const char* line : lines) {
        render::drawStar(Vector2{area.x + theme.px(8.0f), y + theme.px(10.0f)}, theme.px(7.0f),
                         0.0f, palette::ACCENT_GREEN);
        y += theme.drawWrapped(line, Vector2{area.x + theme.px(26.0f), y},
                               area.width - theme.px(26.0f), theme.px(render::textsize::SMALL),
                               palette::TEXT);
        y += theme.px(10.0f);
    }
}

void HelpScreen::draw() {
    const render::Theme& theme = m_context.theme;
    drawMenuBackdrop(m_context);
    drawScreenTitle(theme, "HELP", "");
    const Rectangle panel = contentPanel(theme);
    drawContentPanel(theme, panel);
    const float padding = theme.px(PANEL_PADDING);
    const Rectangle area{panel.x + padding, panel.y + padding, panel.width - 2.0f * padding,
                         panel.height - 2.0f * padding};
    switch (m_tabs.active()) {
        case 0:
            drawControls(area);
            break;
        case 1:
            drawPerks(area);
            break;
        case 2:
            drawTiers(area);
            break;
        default:
            drawStars(area);
            break;
    }
    m_focus.draw(theme, m_context.time);
    drawFooterHint(theme, "LEFT / RIGHT switch page     ESC back");
}

} // namespace ui
