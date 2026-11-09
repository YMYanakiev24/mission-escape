#pragma once

#include "core/Cell.h"
#include "render/Theme.h"

#include <raylib.h>

#include <string>
#include <vector>

/// @file Hud.h
/// Heads-up display: level header, timer, star target, inventory, effects and tooltips.

namespace render {

/// One active timed effect shown with a countdown bar.
struct HudEffect {
    std::string name;
    float remaining = 0.0f;
    float duration = 1.0f;
    Color color{};
};

/// Plain data the HUD displays; filled by the game session every frame.
struct HudData {
    int level = 1;
    std::string tierName;
    Color tierAccent{};
    std::string modeName;
    float elapsedSeconds = 0.0f;
    bool timerStarted = false;
    bool timerFrozen = false;
    int targetStars = 3;             ///< Best rating still reachable.
    float targetLimitSeconds = 0.0f; ///< Time limit of that rating (unused for 1 star).
    float targetSecondsLeft = 0.0f;  ///< Seconds left for that rating.
    float bestSeconds = 0.0f;        ///< Personal best on this level (0 = not completed yet).
    core::Cell playerCell;
    int berries = 0;
    int berryCapacity = 0;
    std::vector<HudEffect> effects;
    std::string hint; ///< Control reminder at the bottom of the screen.
};

/// Draws the HUD; stateless apart from layout constants.
class Hud {
public:
    /// Height of the top bar at the current UI scale.
    static float topBarHeight(const Theme& theme);

    /// Height of the bottom hint strip at the current UI scale.
    static float bottomBarHeight(const Theme& theme);

    /// Draws the top bar, the effect list and the bottom hint strip.
    void draw(const Theme& theme, const HudData& data, float time) const;

    /// Draws a tooltip box near @p anchor with a title and a description.
    void drawTooltip(const Theme& theme, Vector2 anchor, const std::string& title,
                     const std::string& body) const;

private:
    void drawHeader(const Theme& theme, const HudData& data) const;
    void drawTimer(const Theme& theme, const HudData& data, float time) const;
    void drawStatus(const Theme& theme, const HudData& data) const;
    void drawEffects(const Theme& theme, const HudData& data) const;
    void drawHint(const Theme& theme, const HudData& data) const;
};

/// Draws @p count small stars side by side, filled up to @p filled; returns the total width.
float drawStarRow(Vector2 topLeft, float starSize, int count, int filled, Color fillColor,
                  Color emptyColor);

} // namespace render
