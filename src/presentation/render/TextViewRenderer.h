#pragma once

#include "render/GpuResources.h"
#include "render/Theme.h"

#include <raylib.h>

#include <string>

/// @file TextViewRenderer.h
/// Displays the ASCII maze produced by core::mazeToText() on a fixed character grid.

namespace render {

/// Draws the text view inside a panel. The text is re-rasterised into a cached texture only
/// when it changes (i.e. after a move), so even a 101 x 101 character view is cheap per frame.
class TextViewRenderer {
public:
    /// Draws @p text (lines separated by '\n') centred inside @p area.
    void draw(const Theme& theme, const std::string& text, Rectangle area);

private:
    void rasterise(const Theme& theme, const std::string& text, float glyphSize);

    std::string m_cachedText;
    float m_cachedGlyphSize = 0.0f;
    RenderTarget m_cache;
};

/// Colour used for one text-view character.
Color textViewColor(char symbol);

} // namespace render
