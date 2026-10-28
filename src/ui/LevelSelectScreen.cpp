#include "ui/LevelSelectScreen.h"

#include "core/LevelRules.h"
#include "render/Hud.h"
#include "render/Palette.h"
#include "render/Primitives.h"
#include "render/TierStyle.h"
#include "ui/MenuStyle.h"

#include <raylib.h>

#include <string>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float LABEL_COLUMN_WIDTH = 210.0f;
constexpr float TILE_WIDTH = 112.0f;
constexpr float TILE_HEIGHT = 92.0f;
constexpr float TILE_GAP = 16.0f;
constexpr float ROW_GAP = 16.0f;
constexpr float GRID_TOP = 160.0f;
constexpr float BUTTON_WIDTH = 260.0f;
constexpr float BUTTON_HEIGHT = 50.0f;
constexpr float TILE_STAR_SIZE = 7.0f;

float gridWidth() {
    return LABEL_COLUMN_WIDTH + static_cast<float>(core::LEVELS_PER_TIER) * TILE_WIDTH +
           static_cast<float>(core::LEVELS_PER_TIER - 1) * TILE_GAP;
}

} // namespace

// ------------------------------------------------------------------------------------------
// LevelTile
// ------------------------------------------------------------------------------------------

void LevelTile::configure(int level, bool unlocked, int bestStars, Color accent) {
    m_level = level;
    m_bestStars = bestStars;
    m_accent = accent;
    setEnabled(unlocked);
}

bool LevelTile::handleInput(const app::InputState& input, bool focused) {
    if (!m_enabled) {
        return false;
    }
    return (input.mousePressed && contains(input.mouse)) || (focused && input.confirm);
}

void LevelTile::draw(const render::Theme& theme, bool focused, float time) const {
    const Vector2 center{m_bounds.x + m_bounds.width / 2.0f, m_bounds.y + m_bounds.height / 2.0f};
    if (focused) {
        render::drawGlow(center, m_bounds.width * 0.9f,
                         render::fade(m_accent, 0.25f + 0.1f * render::pulse(time, 1.5f)));
    }
    const Color fill = m_enabled ? (focused ? palette::PANEL_HI : palette::PANEL)
                                 : render::fade(palette::BG_DEEP, 0.85f);
    const Color border = focused ? palette::ACCENT_GREEN
                                 : (m_enabled ? render::fade(m_accent, 0.7f)
                                              : render::fade(palette::TEXT_MUTED, 0.25f));
    render::drawPanel(m_bounds, theme.px(10.0f), fill, border, theme.px(focused ? 3.0f : 1.5f));
    if (!m_enabled) {
        render::drawLockIcon(Vector2{center.x, center.y - theme.px(6.0f)}, theme.px(30.0f),
                             render::fade(palette::TEXT_MUTED, 0.6f));
        theme.drawTextCentered(std::to_string(m_level),
                               Vector2{center.x, m_bounds.y + m_bounds.height - theme.px(16.0f)},
                               theme.px(render::textsize::TINY),
                               render::fade(palette::TEXT_MUTED, 0.6f));
        return;
    }
    theme.drawTextCentered(std::to_string(m_level), Vector2{center.x, center.y - theme.px(10.0f)},
                           theme.px(render::textsize::HEADING * 0.8f), palette::TEXT);
    const float starSize = theme.px(TILE_STAR_SIZE);
    const float rowWidth = starSize * 2.2f * 3.0f;
    render::drawStarRow(Vector2{center.x - rowWidth / 2.0f,
                                m_bounds.y + m_bounds.height - starSize * 2.0f - theme.px(9.0f)},
                        starSize, core::MAX_STARS, m_bestStars, palette::ACCENT_GREEN,
                        render::fade(palette::TEXT_MUTED, 0.3f));
}

// ------------------------------------------------------------------------------------------
// LevelSelectScreen
// ------------------------------------------------------------------------------------------

LevelSelectScreen::LevelSelectScreen(app::AppContext& context) : m_context(context) {
    m_focus.setFeedback(&context.uiSounds);
    for (LevelTile& tile : m_tiles) {
        m_focus.add(tile);
    }
    m_focus.add(m_viewMode);
    m_focus.add(m_back);
}

void LevelSelectScreen::onEnter() {
    refreshTiles();
    const core::SaveData* profile = m_context.activeProfile();
    const int focusLevel = profile != nullptr ? profile->progress.highestUnlockedLevel() : 1;
    m_focus.focus(m_tiles[static_cast<std::size_t>(focusLevel - 1)]);
}

void LevelSelectScreen::refreshTiles() {
    const core::SaveData* profile = m_context.activeProfile();
    for (int level = 1; level <= core::LEVEL_COUNT; ++level) {
        const bool unlocked = profile != nullptr && profile->progress.isLevelUnlocked(level);
        const int stars = profile != nullptr ? profile->progress.record(level).bestStars : 0;
        const Color accent = render::tierStyle(core::tierForLevel(level)).accent;
        m_tiles[static_cast<std::size_t>(level - 1)].configure(level, unlocked, stars, accent);
    }
    const core::ViewMode mode = profile != nullptr ? profile->viewMode : core::ViewMode::Classic2D;
    m_viewMode.setLabel("View: " + std::string(core::viewModeName(mode)));
    m_viewMode.setEnabled(profile != nullptr);
}

void LevelSelectScreen::layout() {
    const render::Theme& theme = m_context.theme;
    const float left = (static_cast<float>(GetScreenWidth()) - theme.px(gridWidth())) / 2.0f;
    for (int level = 1; level <= core::LEVEL_COUNT; ++level) {
        const int row = (level - 1) / core::LEVELS_PER_TIER;
        const int column = (level - 1) % core::LEVELS_PER_TIER;
        const float x = left + theme.px(LABEL_COLUMN_WIDTH) +
                        static_cast<float>(column) * theme.px(TILE_WIDTH + TILE_GAP);
        const float y =
            theme.px(GRID_TOP) + static_cast<float>(row) * theme.px(TILE_HEIGHT + ROW_GAP);
        m_tiles[static_cast<std::size_t>(level - 1)].setBounds(
            Rectangle{x, y, theme.px(TILE_WIDTH), theme.px(TILE_HEIGHT)});
    }
    const float buttonY = theme.px(GRID_TOP) +
                          static_cast<float>(core::TIER_COUNT) * theme.px(TILE_HEIGHT + ROW_GAP) +
                          theme.px(10.0f);
    const float centerX = static_cast<float>(GetScreenWidth()) / 2.0f;
    const float width = theme.px(BUTTON_WIDTH);
    m_viewMode.setBounds(
        Rectangle{centerX - width - theme.px(10.0f), buttonY, width, theme.px(BUTTON_HEIGHT)});
    m_back.setBounds(Rectangle{centerX + theme.px(10.0f), buttonY, width, theme.px(BUTTON_HEIGHT)});
}

void LevelSelectScreen::handleAction(const Widget* fired) {
    if (fired == &m_back) {
        m_context.navigator.replaceWith(app::ScreenId::MainMenu);
        return;
    }
    if (fired == &m_viewMode) {
        if (core::SaveData* profile = m_context.activeProfile()) {
            profile->viewMode = profile->viewMode == core::ViewMode::Classic2D
                                    ? core::ViewMode::FirstPerson3D
                                    : core::ViewMode::Classic2D;
            m_context.saveActiveProfile();
            refreshTiles();
        }
        return;
    }
    for (const LevelTile& tile : m_tiles) {
        if (fired == &tile) {
            m_context.startLevel(tile.level());
        }
    }
}

void LevelSelectScreen::update(float deltaSeconds, const app::InputState& input) {
    m_context.menuBackground.update(deltaSeconds);
    if (m_context.activeProfile() == nullptr) {
        m_context.navigator.replaceWith(app::ScreenId::MainMenu);
        return;
    }
    layout();
    if (input.back) {
        m_context.navigator.replaceWith(app::ScreenId::MainMenu);
        return;
    }
    handleAction(m_focus.update(input));
}

void LevelSelectScreen::drawTierLabels() const {
    const render::Theme& theme = m_context.theme;
    for (int tierIndex = 0; tierIndex < core::TIER_COUNT; ++tierIndex) {
        const core::Tier tier = static_cast<core::Tier>(tierIndex);
        const Rectangle firstTile =
            m_tiles[static_cast<std::size_t>(tierIndex * core::LEVELS_PER_TIER)].bounds();
        const float x = firstTile.x - theme.px(LABEL_COLUMN_WIDTH);
        const Color accent = render::tierStyle(tier).accent;
        theme.drawText("TIER " + std::to_string(core::tierNumber(tier)),
                       Vector2{x, firstTile.y + theme.px(14.0f)}, theme.px(render::textsize::SMALL),
                       render::fade(accent, 0.8f));
        theme.drawText(std::string(core::tierName(tier)), Vector2{x, firstTile.y + theme.px(38.0f)},
                       theme.px(render::textsize::LARGE), accent);
    }
}

void LevelSelectScreen::draw() {
    const render::Theme& theme = m_context.theme;
    drawMenuBackdrop(m_context);
    const core::SaveData* profile = m_context.activeProfile();
    std::string subtitle;
    if (profile != nullptr) {
        subtitle = profile->playerName + "  |  " + std::to_string(profile->progress.totalStars()) +
                   " / " + std::to_string(core::LEVEL_COUNT * core::MAX_STARS) + " stars  |  " +
                   std::to_string(profile->mazeSize) + " x " + std::to_string(profile->mazeSize);
    }
    drawScreenTitle(theme, "LEVEL SELECT", subtitle);
    drawTierLabels();
    m_focus.draw(theme, m_context.time);
    drawFooterHint(theme, "ARROWS choose a level     ENTER play     ESC main menu");
}

} // namespace ui
