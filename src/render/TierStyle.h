#pragma once

#include "core/LevelRules.h"
#include "render/Palette.h"
#include "render/Primitives.h"

#include <raylib.h>

/// @file TierStyle.h
/// Per-tier accent colours, all derived from the palette tokens.

namespace render {

/// Colours that give each tier its own mood while staying inside the palette.
struct TierStyle {
    Color accent;   ///< Titles, HUD highlights.
    Color floor;    ///< Maze floor fill.
    Color gridLine; ///< Subtle floor grid.
    Color fog;      ///< Colour of the fog of war.
};

/// Style for @p tier: green (1), purple (2), deep blue fog (3), green with pink danger (4).
inline TierStyle tierStyle(core::Tier tier) {
    switch (tier) {
        case core::Tier::Awakening:
            return TierStyle{palette::ACCENT_GREEN, palette::BG, fade(palette::PANEL_HI, 0.55f),
                             palette::BG_DEEP};
        case core::Tier::Labyrinth:
            return TierStyle{palette::WALL_GLOW, palette::BG, fade(palette::WALL, 0.28f),
                             palette::BG_DEEP};
        case core::Tier::Fog:
            return TierStyle{palette::ACCENT_BLUE, mix(palette::BG, palette::PANEL, 0.45f),
                             fade(palette::ACCENT_BLUE, 0.18f),
                             mix(palette::BG_DEEP, palette::ACCENT_BLUE, 0.05f)};
        case core::Tier::Hunted:
            return TierStyle{palette::ACCENT_GREEN, palette::BG, fade(palette::PANEL_HI, 0.5f),
                             palette::BG_DEEP};
    }
    return TierStyle{palette::ACCENT_GREEN, palette::BG, palette::PANEL_HI, palette::BG_DEEP};
}

} // namespace render
