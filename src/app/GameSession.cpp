#include "app/GameSession.h"

#include "app/AppContext.h"
#include "core/Config.h"
#include "core/Scoring.h"
#include "core/TextView.h"
#include "render/Palette.h"
#include "render/PerkIcons.h"
#include "render/Primitives.h"
#include "render/TierStyle.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace app {

namespace palette = render::palette;

namespace {

/// Maps the four movement actions to compass directions in 2D.
struct MoveBinding {
    GameAction action;
    core::Direction direction;
};

constexpr std::array<MoveBinding, 4> MOVE_BINDINGS_2D = {
    MoveBinding{GameAction::Up, core::Direction::North},
    MoveBinding{GameAction::Down, core::Direction::South},
    MoveBinding{GameAction::Left, core::Direction::West},
    MoveBinding{GameAction::Right, core::Direction::East}};

constexpr float TEXT_VIEW_MARGIN = 12.0f;
constexpr float MILLISECONDS_PER_SECOND = 1000.0f;
constexpr float SHAKE_SECONDS = 0.55f;
constexpr float SHAKE_PIXELS = 14.0f;
constexpr float VIGNETTE_DEPTH = 0.22f;
constexpr float HEARTBEAT_SLOW_SECONDS = 1.15f;
constexpr float HEARTBEAT_FAST_SECONDS = 0.42f;

constexpr float PARTICLE_CELL_PIXELS_3D = 42.0f;
constexpr float SHAKE_WORLD_PER_PIXEL = 0.006f;
constexpr float EVENT_SCREEN_HEIGHT_3D = 0.62f;
constexpr const char* HINT_2D =
    "ARROWS / WASD  move     SHIFT  berry     T  text view     R  restart     ESC  pause";
constexpr const char* HINT_3D =
    "W S  walk     A D  turn     Q E  strafe     SHIFT  berry     M  map     T  text view     "
    "R  restart     ESC  pause";

std::string cellLabel(core::Cell cell) {
    return "Cell (" + std::to_string(cell.x) + ", " + std::to_string(cell.y) + "). ";
}

/// Executes movement keys: a fresh press moves now (or is buffered behind the running move),
/// a held key keeps walking as soon as the previous move has finished.
void applyMovementBindings(core::Level& level, FeedbackPresenter& feedback,
                           const std::array<MoveBinding, 4>& bindings) {
    for (const MoveBinding& binding : bindings) {
        if (isActionPressed(binding.action)) {
            if (level.player().isMoving()) {
                level.bufferMove(binding.direction);
            } else if (level.tryMove(binding.direction) == core::MoveResult::Blocked) {
                feedback.bump();
            }
            return;
        }
    }
    if (level.player().isMoving()) {
        return;
    }
    for (const MoveBinding& binding : bindings) {
        if (isActionDown(binding.action) &&
            level.tryMove(binding.direction) == core::MoveResult::Moved) {
            return;
        }
    }
}

} // namespace

GameSession::GameSession(AppContext& context, int levelNumber, int mazeSize, std::uint32_t seed,
                         core::ViewMode mode)
    : m_context(context), m_mazeSize(mazeSize),
      m_level(std::make_unique<core::Level>(levelNumber, mazeSize, seed, mode)),
      m_feedback(context, m_effects) {
    beginPresentation();
}

void GameSession::beginPresentation() {
    m_renderer2D.beginLevel();
    m_minimap.reset();
    if (isFirstPerson()) {
        m_renderer3D.beginLevel(*m_level); // builds the wall mesh once for this maze
    }
}

void GameSession::restart() {
    m_level = std::make_unique<core::Level>(m_level->levelNumber(), m_mazeSize, m_level->seed(),
                                            m_level->viewMode());
    beginPresentation();
    m_effects.clear();
    m_secondsSinceFinished = 0.0f;
    m_hoverCell.reset();
}

bool GameSession::restoreSnapshot(const core::LevelSnapshot& saved) {
    if (!m_level->restore(saved)) {
        return false;
    }
    beginPresentation();
    return true;
}

void GameSession::update(float deltaSeconds, const InputState& input) {
    m_mouse = input.mouse;
    handleToggleInput();
    if (!m_level->isFinished()) {
        handleAbilityInput();
        handleMovementInput();
    }
    m_level->update(deltaSeconds);
    processEvents();
    updateHeartbeat(deltaSeconds);
    if (m_level->isFinished()) {
        m_secondsSinceFinished += deltaSeconds;
    }
    m_shakeSeconds = std::max(0.0f, m_shakeSeconds - deltaSeconds);
    m_effects.update(deltaSeconds);
    if (!m_context.lastPersistenceError.empty()) {
        m_effects.toast(m_context.lastPersistenceError, palette::WALL_GLOW);
        m_context.lastPersistenceError.clear();
    }
    if (const core::SaveData* profile = m_context.activeProfile()) {
        m_renderer2D.setSkin(profile->progress.selectedSkin());
    }
    m_renderer2D.setShake(shakeOffset());
    const Vector2 shake = shakeOffset();
    m_renderer3D.setShake(
        Vector2{shake.x * SHAKE_WORLD_PER_PIXEL, shake.y * SHAKE_WORLD_PER_PIXEL});
    if (isFirstPerson()) {
        m_renderer3D.update(deltaSeconds, *m_level, m_context.settings.headBob);
    } else {
        m_renderer2D.update(deltaSeconds, *m_level, viewport(), m_context.theme.scale());
    }
    updateHover(input);
}

void GameSession::refreshView() {
    m_hoverCell.reset(); // no tooltips while a menu covers the game
    m_renderer2D.setShake(Vector2{0.0f, 0.0f});
    m_renderer3D.setShake(Vector2{0.0f, 0.0f});
    if (isFirstPerson()) {
        m_renderer3D.update(0.0f, *m_level, m_context.settings.headBob);
    } else {
        m_renderer2D.update(0.0f, *m_level, viewport(), m_context.theme.scale());
    }
}

void GameSession::handleMovementInput() {
    if (isFirstPerson()) {
        handleMovementInput3D();
        return;
    }
    applyMovementBindings(*m_level, m_feedback, MOVE_BINDINGS_2D);
}

void GameSession::handleMovementInput3D() {
    // Left/right turn on the spot; every move is relative to where the player looks.
    if (isActionPressed(GameAction::Left)) {
        m_level->turnLeft();
    }
    if (isActionPressed(GameAction::Right)) {
        m_level->turnRight();
    }
    const core::Direction facing = m_level->player().facing();
    const std::array<MoveBinding, 4> bindings = {
        MoveBinding{GameAction::Up, facing}, MoveBinding{GameAction::Down, core::opposite(facing)},
        MoveBinding{GameAction::StrafeLeft, core::turnedLeft(facing)},
        MoveBinding{GameAction::StrafeRight, core::turnedRight(facing)}};
    applyMovementBindings(*m_level, m_feedback, bindings);
}

void GameSession::handleToggleInput() {
    if (isActionPressed(GameAction::ToggleTextView)) {
        m_showTextView = !m_showTextView;
    }
    if (isFirstPerson() && isActionPressed(GameAction::ToggleMinimap)) {
        m_minimap.toggleSize();
    }
}

void GameSession::handleAbilityInput() {
    if (isActionPressed(GameAction::UseBerry)) {
        m_level->activateBerry(); // success and failure are both reported as events
    }
}

Vector2 GameSession::cellToScreen(core::Cell cell) const {
    if (isFirstPerson()) {
        // Events happen where the player stands, i.e. just in front of the camera.
        return Vector2{static_cast<float>(GetScreenWidth()) / 2.0f,
                       static_cast<float>(GetScreenHeight()) * EVENT_SCREEN_HEIGHT_3D};
    }
    return m_renderer2D.toScreen(
        core::Vec2f{static_cast<float>(cell.x), static_cast<float>(cell.y)});
}

void GameSession::processEvents() {
    const float cellPixels =
        isFirstPerson() ? m_context.theme.px(PARTICLE_CELL_PIXELS_3D) : m_renderer2D.cellSize();
    for (const core::LevelEvent& event : m_level->takeEvents()) {
        if (event.type == core::LevelEventType::Caught && m_context.settings.screenShake) {
            m_shakeSeconds = SHAKE_SECONDS;
        }
        m_feedback.present(event, cellToScreen(event.cell), cellPixels);
    }
}

void GameSession::updateHover(const InputState& input) {
    m_hoverCell.reset();
    if (!m_showTextView && !isFirstPerson()) { // tooltips belong to the top-down view
        m_hoverCell = m_renderer2D.cellAt(input.mouse);
    }
}

Rectangle GameSession::viewport() const {
    const float top = render::Hud::topBarHeight(m_context.theme);
    const float bottom = render::Hud::bottomBarHeight(m_context.theme);
    return Rectangle{0.0f, top, static_cast<float>(GetScreenWidth()),
                     static_cast<float>(GetScreenHeight()) - top - bottom};
}

render::HudData GameSession::buildHudData() const {
    const core::Level& level = *m_level;
    render::HudData data;
    data.level = level.levelNumber();
    data.tierName = std::string(core::tierName(level.spec().tier));
    data.tierAccent = render::tierStyle(level.spec().tier).accent;
    data.modeName = std::string(core::viewModeName(level.viewMode()));
    data.elapsedSeconds = level.elapsedSeconds();
    data.timerStarted = level.timerStarted();
    const core::StarTarget target =
        core::starTargetAt(level.elapsedSeconds(), level.parTimeSeconds());
    data.targetStars = target.stars;
    data.targetLimitSeconds = core::starTimeLimit(target.stars, level.parTimeSeconds());
    data.targetSecondsLeft = target.secondsLeft;
    if (const core::SaveData* profile = m_context.activeProfile()) {
        const core::LevelRecord& record = profile->progress.record(level.levelNumber());
        if (record.completed) {
            data.bestSeconds = static_cast<float>(record.bestTimeMs) / MILLISECONDS_PER_SECOND;
        }
    }
    data.playerCell = level.player().cell();
    data.berries = level.player().berries();
    data.berryCapacity = core::BERRY_CAPACITY;
    data.hint = isFirstPerson() ? HINT_3D : HINT_2D;
    if (level.spec().hasChaser && !level.chaser().isActive()) {
        data.effects.push_back(render::HudEffect{
            level.timerStarted() ? "The Hunter wakes up" : "The Hunter wakes after your first move",
            level.chaserGraceRemaining(), core::CHASER_GRACE_SECONDS, palette::DANGER});
    }
    for (const core::EffectType type : core::ALL_EFFECT_TYPES) {
        if (level.effects().isActive(type)) {
            data.effects.push_back(render::HudEffect{
                std::string(core::effectName(type)), level.effects().remaining(type),
                core::effectDuration(type), render::effectColor(type)});
        }
    }
    data.timerFrozen = level.effects().isActive(core::EffectType::TimeFreeze);
    return data;
}

std::string GameSession::buildTextView() const {
    const core::Level& level = *m_level;
    core::TextViewOptions options;
    const bool fogged = level.spec().hasFog;
    if (fogged) {
        options.explored = &level.fog().explored();
    }
    for (const core::PerkItem& perk : level.perks()) {
        if (!perk.collected && (!fogged || level.fog().isExplored(perk.cell))) {
            options.markers.push_back(core::TextMarker{perk.cell, core::TEXT_PERK});
        }
    }
    const core::Chaser& chaser = level.chaser();
    if (chaser.isActive() && (!fogged || level.fog().isVisible(chaser.cell()))) {
        options.markers.push_back(core::TextMarker{chaser.cell(), core::TEXT_CHASER});
    }
    options.markers.push_back(core::TextMarker{level.player().cell(), core::TEXT_PLAYER});
    return core::mazeToText(level.maze(), options);
}

void GameSession::updateHeartbeat(float deltaSeconds) {
    const float danger = m_level->chaserDanger();
    if (danger <= 0.0f || m_level->isFinished()) {
        m_heartbeatSeconds = 0.0f;
        return;
    }
    // The closer the Hunter, the louder and faster the heart beats.
    m_heartbeatSeconds -= deltaSeconds;
    if (m_heartbeatSeconds <= 0.0f) {
        m_context.audio.play(audio::SoundId::Heartbeat, 0.35f + 0.65f * danger);
        m_heartbeatSeconds =
            HEARTBEAT_SLOW_SECONDS - (HEARTBEAT_SLOW_SECONDS - HEARTBEAT_FAST_SECONDS) * danger;
    }
}

Vector2 GameSession::shakeOffset() const {
    if (m_shakeSeconds <= 0.0f) {
        return Vector2{0.0f, 0.0f};
    }
    const float strength = m_context.theme.px(SHAKE_PIXELS) * (m_shakeSeconds / SHAKE_SECONDS);
    const float time = m_context.time;
    return Vector2{std::sin(time * 71.0f) * strength, std::cos(time * 53.0f) * strength};
}

void GameSession::drawDangerVignette() const {
    const float danger = m_level->chaserDanger();
    if (danger <= 0.0f) {
        return;
    }
    // The pulse speeds up like a heartbeat as the chaser closes in.
    const float beat = render::pulse(m_context.time, 0.8f + 1.6f * danger);
    const Color edge = render::fade(palette::DANGER, danger * (0.30f + 0.25f * beat));
    const Color clear = render::fade(palette::DANGER, 0.0f);
    const Rectangle area = viewport();
    const int depth = static_cast<int>(std::min(area.width, area.height) * VIGNETTE_DEPTH);
    const int x = static_cast<int>(area.x);
    const int y = static_cast<int>(area.y);
    const int width = static_cast<int>(area.width);
    const int height = static_cast<int>(area.height);
    DrawRectangleGradientV(x, y, width, depth, edge, clear);
    DrawRectangleGradientV(x, y + height - depth, width, depth, clear, edge);
    DrawRectangleGradientH(x, y, depth, height, edge, clear);
    DrawRectangleGradientH(x + width - depth, y, depth, height, clear, edge);
}

CellDescription GameSession::describeCell(core::Cell cell) const {
    const core::Maze& maze = m_level->maze();
    if (m_level->spec().hasFog && !m_level->fog().isExplored(cell)) {
        return {"Unexplored", cellLabel(cell) + "Hidden in the fog. Walk closer to see it."};
    }
    if (m_level->chaser().isActive() && cell == m_level->chaser().cell()) {
        return {"The Hunter",
                cellLabel(cell) + "It follows the shortest path to you. Keep moving!"};
    }
    for (const core::PerkItem& perk : m_level->perks()) {
        if (!perk.collected && perk.cell == cell) {
            return {std::string(core::perkName(perk.type)),
                    cellLabel(cell) + std::string(core::perkDescription(perk.type))};
        }
    }
    if (cell == maze.exit()) {
        return {"Exit portal", cellLabel(cell) + "Step inside to finish the level."};
    }
    if (cell == maze.start()) {
        return {"Start", cellLabel(cell) + "Every run begins in the top-left corner."};
    }
    for (const core::Direction direction : core::ALL_DIRECTIONS) {
        if (maze.hasSecretDoor(cell, direction)) {
            return {"Secret door", cellLabel(cell) + "A green rune on the " +
                                       std::string(core::directionName(direction)) +
                                       " wall: you can walk straight through it."};
        }
    }
    return {"Path", cellLabel(cell) + "An open corridor cell."};
}

void GameSession::drawWorld(float time) {
    if (!isFirstPerson()) {
        ClearBackground(palette::BG_DEEP);
        m_renderer2D.draw(*m_level, time);
        return;
    }
    m_renderer3D.draw(*m_level, time);
    const core::SkinId skin = m_context.activeProfile() != nullptr
                                  ? m_context.activeProfile()->progress.selectedSkin()
                                  : core::SkinId::Wisp;
    m_renderer3D.drawHandLight(skin, *m_level, time);
    m_minimap.draw(m_context.theme, *m_level, m_renderer3D.yaw(), skin, time);
}

void GameSession::draw() {
    const float time = m_context.time;
    drawWorld(time);
    m_effects.drawParticles();
    drawDangerVignette();
    if (m_showTextView) {
        const Rectangle area = viewport();
        const float margin = m_context.theme.px(TEXT_VIEW_MARGIN);
        m_textView.draw(m_context.theme, buildTextView(),
                        Rectangle{area.x + margin, area.y + margin, area.width - 2.0f * margin,
                                  area.height - 2.0f * margin});
    }
    m_hud.draw(m_context.theme, buildHudData(), time);
    m_effects.drawToasts(m_context.theme,
                         render::Hud::topBarHeight(m_context.theme) + m_context.theme.px(14.0f));
    if (m_hoverCell) {
        const CellDescription description = describeCell(*m_hoverCell);
        m_hud.drawTooltip(m_context.theme, m_mouse, description.title, description.body);
    }
}

} // namespace app
