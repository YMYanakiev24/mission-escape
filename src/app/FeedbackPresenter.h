#pragma once

#include "core/Level.h"
#include "render/Effects.h"

#include <raylib.h>

/// @file FeedbackPresenter.h
/// Turns gameplay events reported by the core into particles, toasts and sounds.

namespace app {

struct AppContext;

/// Presents core::LevelEvent values to the player. The core only reports *what* happened;
/// this class decides how it looks and sounds.
class FeedbackPresenter {
public:
    /// Uses @p context for audio and @p effects for particles and toasts.
    FeedbackPresenter(AppContext& context, render::Effects& effects);

    /// Reacts to one event that happened at @p screenPosition (a cell is @p cellPixels wide).
    void present(const core::LevelEvent& event, Vector2 screenPosition, float cellPixels);

    /// Reacts to a blocked move (wall bump).
    void bump();

private:
    void presentPerk(core::PerkType perk, Vector2 screenPosition, float cellPixels);

    AppContext& m_context;
    render::Effects& m_effects;
    bool m_stepToggle = false;
};

} // namespace app
