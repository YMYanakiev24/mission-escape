#include "render/Hud.h"

#include "core/Scoring.h"
#include "render/Palette.h"
#include "render/PerkIcons.h"
#include "render/Primitives.h"

#include <algorithm>
#include <string>

namespace render {

namespace {

constexpr float TOP_BAR_HEIGHT = 66.0f;
constexpr float BOTTOM_BAR_HEIGHT = 30.0f;
constexpr float EDGE_PADDING = 22.0f;
constexpr float STAR_SIZE = 9.0f;
constexpr float BERRY_RADIUS = 9.0f;
constexpr float EFFECT_BAR_WIDTH = 190.0f;
constexpr float EFFECT_BAR_HEIGHT = 7.0f;
constexpr float EFFECT_ROW_HEIGHT = 34.0f;
constexpr float TOOLTIP_PADDING = 12.0f;
constexpr float TOOLTIP_MAX_WIDTH = 300.0f;

} // namespace

float Hud::topBarHeight(const Theme& theme) {
    return theme.px(TOP_BAR_HEIGHT);
}

float Hud::bottomBarHeight(const Theme& theme) {
    return theme.px(BOTTOM_BAR_HEIGHT);
}

void Hud::draw(const Theme& theme, const HudData& data, float time) const {
    const float width = static_cast<float>(GetScreenWidth());
    const float barHeight = topBarHeight(theme);
    DrawRectangleRec(Rectangle{0.0f, 0.0f, width, barHeight}, fade(palette::PANEL, 0.94f));
    DrawRectangleRec(Rectangle{0.0f, barHeight - theme.px(2.0f), width, theme.px(2.0f)},
                     fade(data.tierAccent, 0.55f));
    drawHeader(theme, data);
    drawTimer(theme, data, time);
    drawStatus(theme, data);
    drawEffects(theme, data);
    drawHint(theme, data);
}

void Hud::drawHeader(const Theme& theme, const HudData& data) const {
    const float x = theme.px(EDGE_PADDING);
    theme.drawText("LEVEL " + std::to_string(data.level), Vector2{x, theme.px(8.0f)},
                   theme.px(textsize::LARGE), palette::TEXT);
    const std::string subtitle = data.tierName + "  |  " + data.modeName;
    const float subtitleSize = theme.px(textsize::SMALL);
    theme.drawText(subtitle, Vector2{x, theme.px(40.0f)}, subtitleSize, data.tierAccent);
    if (data.bestSeconds <= 0.0f) {
        return;
    }
    // Green while the current attempt is still ahead of the personal best.
    const bool onRecordPace = data.elapsedSeconds < data.bestSeconds;
    const std::string best = "  |  BEST " + core::formatDuration(data.bestSeconds);
    theme.drawText(best, Vector2{x + theme.measure(subtitle, subtitleSize).x, theme.px(40.0f)},
                   subtitleSize, onRecordPace ? palette::ACCENT_GREEN : palette::TEXT_MUTED);
}

void Hud::drawTimer(const Theme& theme, const HudData& data, float time) const {
    const float centerX = static_cast<float>(GetScreenWidth()) / 2.0f;
    const float timerSize = theme.px(textsize::HEADING * 0.86f);
    const std::string timeText = core::formatDuration(data.elapsedSeconds);
    const float timerWidth = theme.tabularWidth(timeText, timerSize);
    Color timerColor = data.timerStarted ? palette::TEXT : palette::TEXT_MUTED;
    if (data.timerFrozen) {
        timerColor = mix(palette::ACCENT_BLUE, palette::TEXT, 0.3f * pulse(time, 2.0f));
    }
    theme.drawTabular(timeText, Vector2{centerX - timerWidth / 2.0f, theme.px(2.0f)}, timerSize,
                      timerColor);

    const float rowY = theme.px(44.0f);
    const float smallSize = theme.px(textsize::TINY);
    if (!data.timerStarted) {
        theme.drawTextCentered("Move to start the timer", Vector2{centerX, rowY + theme.px(6.0f)},
                               smallSize, palette::TEXT_MUTED);
        return;
    }
    const std::string label = data.targetStars > 1
                                  ? "under " + core::formatDuration(data.targetLimitSeconds)
                                  : "finish to earn";
    const float starSize = theme.px(STAR_SIZE);
    const float starsWidth = starSize * 2.2f * 3.0f;
    const float labelWidth = theme.measure(label, smallSize).x;
    const float startX = centerX - (starsWidth + theme.px(8.0f) + labelWidth) / 2.0f;
    drawStarRow(Vector2{startX, rowY}, starSize, 3, data.targetStars, palette::ACCENT_GREEN,
                fade(palette::TEXT_MUTED, 0.35f));
    const bool urgent = data.targetStars > 1 && data.targetSecondsLeft < 10.0f;
    theme.drawText(label, Vector2{startX + starsWidth + theme.px(8.0f), rowY + theme.px(1.0f)},
                   smallSize, urgent ? palette::WALL_GLOW : palette::TEXT_MUTED);
}

void Hud::drawStatus(const Theme& theme, const HudData& data) const {
    const float right = static_cast<float>(GetScreenWidth()) - theme.px(EDGE_PADDING);
    const std::string cellText =
        "CELL " + std::to_string(data.playerCell.x) + ", " + std::to_string(data.playerCell.y);
    theme.drawTextRight(cellText, Vector2{right, theme.px(10.0f)}, theme.px(textsize::SMALL),
                        palette::TEXT);
    if (data.berryCapacity <= 0) {
        return;
    }
    const float radius = theme.px(BERRY_RADIUS);
    const float spacing = radius * 2.6f;
    const float iconY = theme.px(46.0f);
    for (int slot = 0; slot < data.berryCapacity; ++slot) {
        const float x =
            right - radius - static_cast<float>(data.berryCapacity - 1 - slot) * spacing;
        drawBerryIcon(Vector2{x, iconY}, radius, slot < data.berries, 0.0f);
    }
    const float labelRight = right - static_cast<float>(data.berryCapacity) * spacing;
    theme.drawTextRight(data.berryKey, Vector2{labelRight, iconY - theme.px(8.0f)},
                        theme.px(textsize::TINY), palette::TEXT_MUTED);
}

void Hud::drawEffects(const Theme& theme, const HudData& data) const {
    float y = topBarHeight(theme) + theme.px(12.0f);
    const float x = theme.px(EDGE_PADDING);
    for (const HudEffect& effect : data.effects) {
        const Rectangle panel{x - theme.px(8.0f), y - theme.px(4.0f),
                              theme.px(EFFECT_BAR_WIDTH + 16.0f),
                              theme.px(EFFECT_ROW_HEIGHT - 4.0f)};
        drawPanel(panel, theme.px(6.0f), fade(palette::PANEL, 0.85f), fade(effect.color, 0.5f),
                  theme.px(1.0f));
        theme.drawText(effect.name, Vector2{x, y}, theme.px(textsize::TINY), effect.color);
        const float ratio =
            std::clamp(effect.remaining / std::max(0.001f, effect.duration), 0.0f, 1.0f);
        const Rectangle track{x, y + theme.px(17.0f), theme.px(EFFECT_BAR_WIDTH),
                              theme.px(EFFECT_BAR_HEIGHT)};
        DrawRectangleRounded(track, 1.0f, 6, palette::BG_DEEP);
        DrawRectangleRounded(Rectangle{track.x, track.y, track.width * ratio, track.height}, 1.0f,
                             6, effect.color);
        y += theme.px(EFFECT_ROW_HEIGHT);
    }
}

void Hud::drawHint(const Theme& theme, const HudData& data) const {
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    const float barHeight = bottomBarHeight(theme);
    DrawRectangleRec(Rectangle{0.0f, height - barHeight, width, barHeight},
                     fade(palette::BG_DEEP, 0.85f));
    theme.drawTextCentered(data.hint, Vector2{width / 2.0f, height - barHeight / 2.0f},
                           theme.px(textsize::TINY), palette::TEXT_MUTED);
}

void Hud::drawTooltip(const Theme& theme, Vector2 anchor, const std::string& title,
                      const std::string& body) const {
    const float padding = theme.px(TOOLTIP_PADDING);
    const float titleSize = theme.px(textsize::SMALL);
    const float bodySize = theme.px(textsize::TINY);
    const float maxWidth = theme.px(TOOLTIP_MAX_WIDTH);
    const float titleWidth = theme.measure(title, titleSize).x;
    const float bodyTop = titleSize * 1.15f;
    // Measure the body exactly as drawWrapped() will lay it out, so the box fits the text
    // instead of guessing a (fractional) line count.
    const Vector2 bodyExtent =
        body.empty() ? Vector2{0.0f, 0.0f} : theme.measureWrapped(body, maxWidth, bodySize);
    const float width = std::max(titleWidth, bodyExtent.x) + 2.0f * padding;
    const float height = padding * 2.0f + (body.empty() ? titleSize : bodyTop + bodyExtent.y);
    Rectangle box{anchor.x + theme.px(18.0f), anchor.y + theme.px(18.0f), width, height};
    // Keep the tooltip on screen.
    box.x = std::min(box.x, static_cast<float>(GetScreenWidth()) - width - theme.px(6.0f));
    box.y = std::min(box.y, static_cast<float>(GetScreenHeight()) - height - theme.px(6.0f));
    drawPanel(box, theme.px(8.0f), fade(palette::PANEL_HI, 0.96f), palette::WALL_GLOW,
              theme.px(1.5f));
    theme.drawText(title, Vector2{box.x + padding, box.y + padding}, titleSize, palette::TEXT);
    if (!body.empty()) {
        theme.drawWrapped(body, Vector2{box.x + padding, box.y + padding + bodyTop}, maxWidth,
                          bodySize, palette::TEXT_MUTED);
    }
}

float drawStarRow(Vector2 topLeft, float starSize, int count, int filled, Color fillColor,
                  Color emptyColor) {
    const float step = starSize * 2.2f;
    for (int index = 0; index < count; ++index) {
        const Vector2 center{topLeft.x + starSize + static_cast<float>(index) * step,
                             topLeft.y + starSize};
        drawStar(center, starSize, 0.0f, index < filled ? fillColor : emptyColor);
    }
    return step * static_cast<float>(count);
}

} // namespace render
