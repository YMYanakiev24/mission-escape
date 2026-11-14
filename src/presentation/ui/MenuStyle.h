#pragma once

#include "app/AppContext.h"
#include "render/Theme.h"

#include <raylib.h>

#include <string_view>

/// @file MenuStyle.h
/// Shared look of the menu screens: backdrop, titles, footers and simple layout helpers.

namespace ui {

/// Draws the animated maze backdrop used behind every menu.
void drawMenuBackdrop(const app::AppContext& context);

/// Draws a glowing screen title and an optional subtitle near the top of the window.
void drawScreenTitle(const render::Theme& theme, std::string_view title, std::string_view subtitle);

/// Draws a control hint at the bottom of the window.
void drawFooterHint(const render::Theme& theme, std::string_view hint);

/// Rectangle of the @p index-th item of a horizontally centred vertical stack.
Rectangle stackedItem(const render::Theme& theme, float width, float height, float top, int index,
                      float gap);

/// Content panel area used by the tabbed screens (Help, Settings).
Rectangle contentPanel(const render::Theme& theme);

/// Draws a translucent rounded panel behind menu content.
void drawContentPanel(const render::Theme& theme, Rectangle bounds);

} // namespace ui
