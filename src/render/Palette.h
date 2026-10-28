#pragma once

#include <raylib.h>

/// @file Palette.h
/// Named colour tokens of the dark blue / purple / green neon style. Every colour drawn by the
/// game comes from these tokens (optionally faded or blended), never from ad-hoc literals.

namespace render::palette {

inline constexpr Color BG_DEEP{0x07, 0x0A, 0x1F, 0xFF};
inline constexpr Color BG{0x0D, 0x12, 0x33, 0xFF};
inline constexpr Color PANEL{0x16, 0x1C, 0x45, 0xFF};
inline constexpr Color PANEL_HI{0x22, 0x2A, 0x5E, 0xFF};
inline constexpr Color WALL{0x5B, 0x3F, 0xD1, 0xFF};
inline constexpr Color WALL_GLOW{0x8E, 0x6B, 0xFF, 0xFF};
inline constexpr Color ACCENT_GREEN{0x36, 0xF2, 0xA0, 0xFF};
inline constexpr Color ACCENT_GREEN_DIM{0x1E, 0x8F, 0x63, 0xFF};
inline constexpr Color ACCENT_BLUE{0x3A, 0x7B, 0xFF, 0xFF};
inline constexpr Color TEXT{0xE7, 0xE9, 0xFF, 0xFF};
inline constexpr Color TEXT_MUTED{0x8C, 0x91, 0xBD, 0xFF};
/// Reserved for the tier 4 chaser and its warnings; use sparingly.
inline constexpr Color DANGER{0xFF, 0x3D, 0x8B, 0xFF};

} // namespace render::palette
