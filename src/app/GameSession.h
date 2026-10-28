#pragma once

#include "app/FeedbackPresenter.h"
#include "app/Input.h"
#include "core/Level.h"
#include "core/LevelRules.h"
#include "render/Effects.h"
#include "render/Hud.h"
#include "render/Minimap.h"
#include "render/Renderer2D.h"
#include "render/Renderer3D.h"
#include "render/TextViewRenderer.h"

#include <raylib.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

/// @file GameSession.h
/// Gameplay controller: turns input into core commands and core state into pictures.

namespace app {

struct AppContext;

/// Hover tooltip content for one maze cell.
struct CellDescription {
    std::string title;
    std::string body;
};

/// Runs one level attempt: owns the core::Level plus everything needed to show it.
///
/// The session never decides game rules itself; it forwards commands such as tryMove() to the
/// core and reacts to the events the core reports (sounds, particles, toasts).
class GameSession {
public:
    /// Creates level @p levelNumber of an N x N maze from @p seed.
    GameSession(AppContext& context, int levelNumber, int mazeSize, std::uint32_t seed,
                core::ViewMode mode);

    /// The simulated level.
    const core::Level& level() const noexcept { return *m_level; }
    /// The simulated level (mutable, used by the screenshot tour).
    core::Level& level() noexcept { return *m_level; }
    /// Side length of the maze.
    int mazeSize() const noexcept { return m_mazeSize; }

    /// Throws the current attempt away and regenerates the same maze (same seed).
    void restart();

    /// Continues a saved attempt of this level; false if the snapshot does not fit.
    bool restoreSnapshot(const core::LevelSnapshot& saved);

    /// Advances gameplay by one frame (not called while paused).
    void update(float deltaSeconds, const InputState& input);

    /// Keeps cameras and caches up to date without advancing gameplay (paused / overlays).
    void refreshView();

    /// Draws the level, the HUD and overlays such as the text view and tooltips.
    void draw();

    /// Switches the 3D minimap between small and large (M).
    void toggleMinimapSize() noexcept { m_minimap.toggleSize(); }

    /// Shows or hides the ASCII text view overlay (T).
    void setTextViewVisible(bool visible) noexcept { m_showTextView = visible; }

    /// Seconds since the level was won or lost (0 while playing).
    float secondsSinceFinished() const noexcept { return m_secondsSinceFinished; }

private:
    bool isFirstPerson() const { return m_level->viewMode() == core::ViewMode::FirstPerson3D; }
    void beginPresentation();
    void handleMovementInput();
    void handleMovementInput3D();
    void handleToggleInput();
    void handleAbilityInput();
    Vector2 cellToScreen(core::Cell cell) const;
    void processEvents();
    void updateHover(const InputState& input);
    Rectangle viewport() const;
    render::HudData buildHudData() const;
    std::string buildTextView() const;
    CellDescription describeCell(core::Cell cell) const;
    Vector2 shakeOffset() const;
    void drawDangerVignette() const;
    void drawWorld(float time);
    void updateHeartbeat(float deltaSeconds);

    AppContext& m_context;
    int m_mazeSize;
    std::unique_ptr<core::Level> m_level;
    render::Renderer2D m_renderer2D;
    render::Renderer3D m_renderer3D;
    render::Minimap m_minimap;
    render::Hud m_hud;
    render::TextViewRenderer m_textView;
    render::Effects m_effects;
    FeedbackPresenter m_feedback;
    bool m_showTextView = false;
    std::optional<core::Cell> m_hoverCell;
    Vector2 m_mouse{};
    float m_secondsSinceFinished = 0.0f;
    float m_shakeSeconds = 0.0f;
    float m_heartbeatSeconds = 0.0f;
};

} // namespace app
