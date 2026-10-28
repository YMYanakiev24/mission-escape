#pragma once

#include <raylib.h>

#include <string>
#include <string_view>

/// @file Theme.h
/// Fonts, UI scaling and text drawing helpers shared by every screen.

namespace render {

/// Base text sizes at the reference resolution (1280 x 720); multiplied by the UI scale.
namespace textsize {
inline constexpr float TITLE = 76.0f;
inline constexpr float HEADING = 42.0f;
inline constexpr float LARGE = 30.0f;
inline constexpr float BODY = 23.0f;
inline constexpr float SMALL = 18.0f;
inline constexpr float TINY = 15.0f;
} // namespace textsize

/// Owns the UI font and the current scale factor; draws text consistently.
///
/// The UI is laid out for 1280 x 720 and scaled uniformly with the window size.
/// If the bundled font cannot be loaded, raylib's built-in font is used instead.
class Theme {
public:
    Theme() = default;
    ~Theme();
    Theme(const Theme&) = delete;
    Theme& operator=(const Theme&) = delete;

    /// Loads the TTF at @p fontPath; falls back to the default font when it is missing.
    void loadFont(const std::string& fontPath);

    /// Releases the font (call before the window closes).
    void unloadFont();

    /// Recomputes the scale factor for a window of the given size.
    void updateScale(int screenWidth, int screenHeight);

    /// Uniform UI scale relative to 1280 x 720.
    float scale() const noexcept { return m_scale; }

    /// Scales a reference-resolution length to the current window.
    float px(float referenceLength) const noexcept { return referenceLength * m_scale; }

    /// True when the bundled TTF font is in use.
    bool hasCustomFont() const noexcept { return m_ownsFont; }

    /// The active font.
    const Font& font() const noexcept { return m_font; }

    /// Size of @p text at @p size pixels.
    Vector2 measure(std::string_view text, float size) const;

    /// Draws text with its top-left corner at @p position.
    void drawText(std::string_view text, Vector2 position, float size, Color color) const;

    /// Draws text centred on @p center.
    void drawTextCentered(std::string_view text, Vector2 center, float size, Color color) const;

    /// Draws text right-aligned so that it ends at @p topRight.
    void drawTextRight(std::string_view text, Vector2 topRight, float size, Color color) const;

    /// Draws centred text with a soft neon glow behind it.
    void drawGlowText(std::string_view text, Vector2 center, float size, Color color,
                      Color glow) const;

    /// Draws text on a fixed character grid (monospace layout) and returns the advance width.
    float drawMonospace(std::string_view text, Vector2 position, float size, Color color) const;

    /// Width of one character cell used by drawMonospace() at @p size.
    float monospaceAdvance(float size) const;

    /// Draws numbers with fixed-width digits and narrow separators (timers do not jitter).
    void drawTabular(std::string_view text, Vector2 position, float size, Color color) const;

    /// Width that drawTabular() needs for @p text.
    float tabularWidth(std::string_view text, float size) const;

    /// Wraps @p text to lines no wider than @p maxWidth and draws them; returns the height used.
    float drawWrapped(std::string_view text, Vector2 position, float maxWidth, float size,
                      Color color) const;

private:
    float spacingFor(float size) const;

    Font m_font{};
    bool m_ownsFont = false;
    float m_scale = 1.0f;
};

} // namespace render
