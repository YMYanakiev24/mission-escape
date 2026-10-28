#pragma once

#include "core/Perks.h"

#include <raylib.h>

/// @file PerkIcons.h
/// Procedurally drawn perk icons shared by the 2D renderer, the HUD and the help screen.

namespace render {

/// Signature colour of a perk (glows, particles, toasts).
Color perkColor(core::PerkType type);

/// Colour of a timed effect (HUD bars, particles).
Color effectColor(core::EffectType type);

/// Draws the icon of @p type centred on @p center; @p time animates it.
void drawPerkIcon(core::PerkType type, Vector2 center, float radius, float time);

/// Draws a Magic Berry icon (filled) or an empty inventory slot.
void drawBerryIcon(Vector2 center, float radius, bool filled, float time);

} // namespace render
