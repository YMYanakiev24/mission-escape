#include "render/TextViewRenderer.h"

#include "core/TextView.h"
#include "render/Palette.h"
#include "render/Primitives.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace render {

namespace {

constexpr float MIN_GLYPH_SIZE = 6.0f;
constexpr float MAX_GLYPH_SIZE = 26.0f;
constexpr float LINE_HEIGHT_RATIO = 1.0f;
constexpr float PANEL_PADDING = 14.0f;

int countLines(const std::string& text) {
    return 1 + static_cast<int>(std::count(text.begin(), text.end(), '\n'));
}

int longestLine(const std::string& text) {
    int longest = 0;
    int current = 0;
    for (const char character : text) {
        current = character == '\n' ? 0 : current + 1;
        longest = std::max(longest, current);
    }
    return longest;
}

} // namespace

Color textViewColor(char symbol) {
    switch (symbol) {
        case core::TEXT_WALL:
            return palette::WALL_GLOW;
        case core::TEXT_PLAYER:
        case core::TEXT_EXIT:
        case core::TEXT_SECRET_DOOR:
            return palette::ACCENT_GREEN;
        case core::TEXT_START:
            return palette::TEXT;
        case core::TEXT_CHASER:
            return palette::DANGER;
        case core::TEXT_PERK:
            return palette::ACCENT_BLUE;
        case core::TEXT_UNKNOWN:
            return fade(palette::TEXT_MUTED, 0.35f);
        default:
            return palette::TEXT_MUTED;
    }
}

void TextViewRenderer::rasterise(const Theme& theme, const std::string& text, float glyphSize) {
    const float advance = theme.monospaceAdvance(glyphSize);
    const float lineHeight = glyphSize * LINE_HEIGHT_RATIO;
    const int width = static_cast<int>(std::ceil(advance * static_cast<float>(longestLine(text))));
    const int height =
        static_cast<int>(std::ceil(lineHeight * static_cast<float>(countLines(text))));
    if (!m_cache.isValid() || m_cache.width() != width || m_cache.height() != height) {
        m_cache = RenderTarget(std::max(1, width), std::max(1, height));
    }
    BeginTextureMode(m_cache.get());
    ClearBackground(BLANK);
    beginPremultipliedAccumulation();
    float x = 0.0f;
    float y = 0.0f;
    for (const char symbol : text) {
        if (symbol == '\n') {
            x = 0.0f;
            y += lineHeight;
            continue;
        }
        if (symbol != ' ') {
            const char glyph[2] = {symbol, '\0'};
            theme.drawMonospace(glyph, Vector2{x, y}, glyphSize, textViewColor(symbol));
        }
        x += advance;
    }
    EndBlendMode();
    EndTextureMode();
    m_cachedText = text;
    m_cachedGlyphSize = glyphSize;
}

void TextViewRenderer::draw(const Theme& theme, const std::string& text, Rectangle area) {
    const float padding = theme.px(PANEL_PADDING);
    const float lines = static_cast<float>(countLines(text));
    const float columns = static_cast<float>(std::max(1, longestLine(text)));
    const float fitByHeight = (area.height - 2.0f * padding) / (lines * LINE_HEIGHT_RATIO);
    const float fitByWidth =
        (area.width - 2.0f * padding) / (columns * theme.monospaceAdvance(1.0f));
    const float glyphSize =
        std::floor(std::clamp(std::min(fitByHeight, fitByWidth), MIN_GLYPH_SIZE, MAX_GLYPH_SIZE));
    if (text != m_cachedText || glyphSize != m_cachedGlyphSize || !m_cache.isValid()) {
        rasterise(theme, text, glyphSize);
    }
    const float width = static_cast<float>(m_cache.width());
    const float height = static_cast<float>(m_cache.height());
    const Rectangle panel{area.x + (area.width - width) / 2.0f - padding,
                          area.y + (area.height - height) / 2.0f - padding, width + 2.0f * padding,
                          height + 2.0f * padding};
    drawPanel(panel, theme.px(10.0f), fade(palette::BG_DEEP, 0.96f), palette::WALL, theme.px(1.5f));
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    DrawTextureRec(m_cache.get().texture, Rectangle{0.0f, 0.0f, width, -height},
                   Vector2{std::round(panel.x + padding), std::round(panel.y + padding)}, WHITE);
    EndBlendMode();
}

} // namespace render
