#pragma once

#include <raylib.h>

/// @file Primitives.h
/// Small procedural drawing helpers (stars, glows, icons) and easing curves shared by the
/// renderers and the UI. Nothing here keeps state.

namespace render {

/// Cubic ease-out: fast start, gentle stop.
float easeOutCubic(float t);

/// Sine ease-in-out for smooth back-and-forth motion.
float easeInOutSine(float t);

/// Back ease-out with a small overshoot, used for "pop" animations.
float easeOutBack(float t);

/// 0..1 sine wave at @p frequency Hz.
float pulse(float timeSeconds, float frequency);

/// Multiplies the colour's alpha by @p factor (clamped to [0, 1]).
Color fade(Color color, float factor);

/// Linear blend between two colours, including alpha.
Color mix(Color from, Color to, float amount);

/// Draws a filled triangle regardless of the vertex winding order.
void drawTriangleAnyWinding(Vector2 a, Vector2 b, Vector2 c, Color color);

/// Draws a filled five-pointed star.
void drawStar(Vector2 center, float radius, float rotationDegrees, Color color);

/// Draws an outlined five-pointed star.
void drawStarOutline(Vector2 center, float radius, float thickness, Color color);

/// Draws a radial glow fading from @p color at the centre to transparent at @p radius.
void drawGlow(Vector2 center, float radius, Color color);

/// Draws a padlock icon centred on @p center.
void drawLockIcon(Vector2 center, float size, Color color);

/// Starts a blend mode that stores premultiplied colour with correct alpha in a render
/// texture. Draw the result later with BLEND_ALPHA_PREMULTIPLY. End with EndBlendMode().
void beginPremultipliedAccumulation();

/// Draws a rounded panel with an optional border.
void drawPanel(Rectangle bounds, float cornerRadius, Color fill, Color border,
               float borderThickness);

} // namespace render
