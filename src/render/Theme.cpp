#include "render/Theme.h"

#include "render/Primitives.h"

#include <algorithm>
#include <vector>

namespace render {

namespace {

constexpr float REFERENCE_WIDTH = 1280.0f;
constexpr float REFERENCE_HEIGHT = 720.0f;
constexpr int FONT_LOAD_SIZE = 72;
constexpr float SPACING_RATIO = 0.03f;
constexpr float MONOSPACE_ADVANCE_RATIO = 0.62f;
constexpr float TABULAR_DIGIT_RATIO = 0.56f;
constexpr float TABULAR_SEPARATOR_RATIO = 0.30f;
constexpr float LINE_HEIGHT_RATIO = 1.25f;
constexpr float GLOW_ALPHA = 0.18f;
constexpr int GLOW_LAYERS = 3;

std::vector<std::string> splitWords(std::string_view text) {
    std::vector<std::string> words;
    std::string current;
    for (const char character : text) {
        if (character == ' ') {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(character);
        }
    }
    if (!current.empty()) {
        words.push_back(current);
    }
    return words;
}

} // namespace

Theme::~Theme() {
    unloadFont();
}

void Theme::loadFont(const std::string& fontPath) {
    unloadFont();
    if (FileExists(fontPath.c_str())) {
        Font loaded = LoadFontEx(fontPath.c_str(), FONT_LOAD_SIZE, nullptr, 0);
        if (IsFontValid(loaded) && loaded.texture.id != 0) {
            GenTextureMipmaps(&loaded.texture);
            SetTextureFilter(loaded.texture, TEXTURE_FILTER_TRILINEAR);
            m_font = loaded;
            m_ownsFont = true;
            return;
        }
    }
    m_font = GetFontDefault();
    m_ownsFont = false;
}

void Theme::unloadFont() {
    if (m_ownsFont) {
        UnloadFont(m_font);
    }
    m_font = Font{};
    m_ownsFont = false;
}

void Theme::updateScale(int screenWidth, int screenHeight) {
    m_scale = std::min(static_cast<float>(screenWidth) / REFERENCE_WIDTH,
                       static_cast<float>(screenHeight) / REFERENCE_HEIGHT);
}

float Theme::spacingFor(float size) const {
    return size * SPACING_RATIO;
}

Vector2 Theme::measure(std::string_view text, float size) const {
    const std::string copy(text);
    return MeasureTextEx(m_font, copy.c_str(), size, spacingFor(size));
}

void Theme::drawText(std::string_view text, Vector2 position, float size, Color color) const {
    const std::string copy(text);
    DrawTextEx(m_font, copy.c_str(), position, size, spacingFor(size), color);
}

void Theme::drawTextCentered(std::string_view text, Vector2 center, float size, Color color) const {
    const Vector2 extent = measure(text, size);
    drawText(text, Vector2{center.x - extent.x / 2.0f, center.y - extent.y / 2.0f}, size, color);
}

void Theme::drawTextRight(std::string_view text, Vector2 topRight, float size, Color color) const {
    const Vector2 extent = measure(text, size);
    drawText(text, Vector2{topRight.x - extent.x, topRight.y}, size, color);
}

void Theme::drawGlowText(std::string_view text, Vector2 center, float size, Color color,
                         Color glow) const {
    // Offset copies in the glow colour build a cheap blur halo behind the crisp text.
    for (int layer = GLOW_LAYERS; layer >= 1; --layer) {
        const float offset = size * 0.035f * static_cast<float>(layer);
        const Color halo = fade(glow, GLOW_ALPHA);
        drawTextCentered(text, Vector2{center.x - offset, center.y}, size, halo);
        drawTextCentered(text, Vector2{center.x + offset, center.y}, size, halo);
        drawTextCentered(text, Vector2{center.x, center.y - offset}, size, halo);
        drawTextCentered(text, Vector2{center.x, center.y + offset}, size, halo);
    }
    drawTextCentered(text, center, size, color);
}

float Theme::monospaceAdvance(float size) const {
    return size * MONOSPACE_ADVANCE_RATIO;
}

float Theme::drawMonospace(std::string_view text, Vector2 position, float size, Color color) const {
    const float advance = monospaceAdvance(size);
    float x = position.x;
    for (const char character : text) {
        const char glyph[2] = {character, '\0'};
        const Vector2 extent = MeasureTextEx(m_font, glyph, size, 0.0f);
        // Centre every glyph inside its fixed-width cell.
        DrawTextEx(m_font, glyph, Vector2{x + (advance - extent.x) / 2.0f, position.y}, size, 0.0f,
                   color);
        x += advance;
    }
    return x - position.x;
}

float Theme::tabularWidth(std::string_view text, float size) const {
    float width = 0.0f;
    for (const char character : text) {
        const bool digit = character >= '0' && character <= '9';
        width += size * (digit ? TABULAR_DIGIT_RATIO : TABULAR_SEPARATOR_RATIO);
    }
    return width;
}

void Theme::drawTabular(std::string_view text, Vector2 position, float size, Color color) const {
    float x = position.x;
    for (const char character : text) {
        const bool digit = character >= '0' && character <= '9';
        const float advance = size * (digit ? TABULAR_DIGIT_RATIO : TABULAR_SEPARATOR_RATIO);
        const char glyph[2] = {character, '\0'};
        const Vector2 extent = MeasureTextEx(m_font, glyph, size, 0.0f);
        DrawTextEx(m_font, glyph, Vector2{x + (advance - extent.x) / 2.0f, position.y}, size, 0.0f,
                   color);
        x += advance;
    }
}

float Theme::drawWrapped(std::string_view text, Vector2 position, float maxWidth, float size,
                         Color color) const {
    const float lineHeight = size * LINE_HEIGHT_RATIO;
    float y = position.y;
    std::string line;
    for (const std::string& word : splitWords(text)) {
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && measure(candidate, size).x > maxWidth) {
            drawText(line, Vector2{position.x, y}, size, color);
            y += lineHeight;
            line = word;
        } else {
            line = candidate;
        }
    }
    if (!line.empty()) {
        drawText(line, Vector2{position.x, y}, size, color);
        y += lineHeight;
    }
    return y - position.y;
}

} // namespace render
