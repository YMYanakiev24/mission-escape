#include "core/Level.h"

#include "core/Config.h"
#include "core/Pathfinding.h"
#include "core/Scoring.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace core {

namespace {

constexpr float MILLISECONDS_PER_SECOND = 1000.0f;

/// In 3D the player starts looking down the first open corridor.
Direction initialFacing(const Maze& maze) {
    return maze.isOpen(maze.start(), Direction::East) ? Direction::East : Direction::South;
}

float distanceBetween(Vec2f a, Vec2f b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

float millisecondsToSeconds(int milliseconds) {
    return static_cast<float>(milliseconds) / MILLISECONDS_PER_SECOND;
}

} // namespace

Level::Level(int levelNumber, int mazeSize, std::uint32_t seed, ViewMode mode)
    : m_spec(levelSpec(levelNumber)), m_mode(mode), m_seed(seed), m_spawnRng(seed) {
    Rng rng(deriveLevelSeed(seed, m_spec.level, mazeSize));
    GeneratedMaze generated = generateLevelMaze(m_spec, mazeSize, rng);
    m_maze = std::move(generated.maze);
    m_secretDoors = std::move(generated.secretDoors);
    m_perks = spawnPerks(m_maze, m_spec.tier, rng);
    m_spawnRng = Rng(rng.nextU32());
    m_player = Player(m_maze.start(), initialFacing(m_maze));
    m_fog = Fog(mazeSize);
    m_shortestPathLength =
        core::shortestPathLength(m_maze, m_maze.start(), m_maze.exit(), PassRules{});
    m_parTime = parTime(m_shortestPathLength, m_spec.tier, m_mode);
    m_graceRemaining = m_spec.hasChaser ? CHASER_GRACE_SECONDS : 0.0f;
    updateVisibility();
}

int Level::fogRadius() const {
    const bool lantern = m_effects.isActive(EffectType::Lantern);
    return FOG_RADIUS * (lantern ? LANTERN_RADIUS_MULTIPLIER : 1);
}

float Level::chaserDanger() const {
    if (!m_chaser.isActive() || m_chaserDistance < 0) {
        return 0.0f;
    }
    const float closeness =
        1.0f - static_cast<float>(m_chaserDistance) / static_cast<float>(CHASER_WARNING_DISTANCE);
    return std::clamp(closeness, 0.0f, 1.0f);
}

MoveResult Level::tryMove(Direction direction) {
    if (isFinished()) {
        return MoveResult::Inactive;
    }
    if (m_player.isMoving()) {
        return MoveResult::Busy;
    }
    if (!m_maze.canPass(m_player.cell(), direction, playerRules())) {
        return MoveResult::Blocked;
    }
    startMove(direction);
    return MoveResult::Moved;
}

void Level::bufferMove(Direction direction) {
    if (!isFinished()) {
        m_bufferedMove = direction;
    }
}

bool Level::activateBerry() {
    if (isFinished() || isPhasing()) {
        return false;
    }
    if (!m_player.consumeBerry()) {
        emit(LevelEventType::NoBerries, m_player.cell());
        return false;
    }
    m_effects.start(EffectType::Phase);
    emit(LevelEventType::PhaseStarted, m_player.cell());
    return true;
}

void Level::turnLeft() {
    m_player.setFacing(turnedLeft(m_player.facing()));
}

void Level::turnRight() {
    m_player.setFacing(turnedRight(m_player.facing()));
}

void Level::update(float deltaSeconds) {
    if (isFinished()) {
        return;
    }
    advanceTimer(deltaSeconds);
    updateEffects(deltaSeconds);
    if (m_player.update(deltaSeconds)) {
        onPlayerArrived();
    }
    if (isFinished()) {
        return;
    }
    updateVisibility();
    updateChaser(deltaSeconds);
    checkCaught();
}

std::vector<LevelEvent> Level::takeEvents() {
    return std::exchange(m_events, {});
}

PassRules Level::playerRules() const {
    // Phasing opens every inner wall; Maze::canPass still keeps the outer boundary closed.
    return PassRules{true, isPhasing()};
}

void Level::startMove(Direction direction) {
    if (!m_timerStarted) {
        m_timerStarted = true;
        emit(LevelEventType::TimerStarted, m_player.cell());
    }
    if (m_mode == ViewMode::Classic2D) {
        m_player.setFacing(direction); // in 3D, strafing and backing up keep the view direction
    }
    const Cell from = m_player.cell();
    if (m_maze.hasSecretDoor(from, direction)) {
        emit(LevelEventType::SecretDoorUsed, from);
    }
    m_player.beginMove(neighbour(from, direction), moveSecondsFor(m_mode));
    emit(LevelEventType::Moved, m_player.cell());
    updateVisibility(); // reveal the destination as soon as the step starts
}

void Level::onPlayerArrived() {
    collectPerkAt(m_player.cell());
    refreshCompassPath();
    if (m_player.cell() == m_maze.exit()) {
        win();
        return;
    }
    if (m_bufferedMove) {
        const Direction buffered = *m_bufferedMove;
        m_bufferedMove.reset();
        tryMove(buffered);
    }
}

void Level::collectPerkAt(Cell cell) {
    for (PerkItem& perk : m_perks) {
        if (perk.collected || perk.cell != cell) {
            continue;
        }
        if (perk.type == PerkType::MagicBerry && !m_player.addBerry()) {
            emit(LevelEventType::InventoryFull, cell); // stays on the floor for later
            continue;
        }
        perk.collected = true;
        applyPerk(perk.type);
        emit(LevelEventType::PerkCollected, cell, perk.type);
    }
}

void Level::applyPerk(PerkType type) {
    switch (type) {
        case PerkType::MagicBerry:
            break; // already added to the inventory
        case PerkType::Hourglass:
            m_effects.start(EffectType::TimeFreeze);
            break;
        case PerkType::CompassSpark:
            m_effects.start(EffectType::CompassTrail);
            break;
        case PerkType::Lantern:
            m_effects.start(EffectType::Lantern);
            break;
        case PerkType::FrostShard:
            m_effects.start(EffectType::ChaserFreeze);
            break;
    }
}

void Level::updateEffects(float deltaSeconds) {
    for (const EffectType ended : m_effects.update(deltaSeconds)) {
        if (ended == EffectType::Phase) {
            emit(LevelEventType::PhaseEnded, m_player.cell());
        }
    }
    refreshCompassPath();
}

void Level::refreshCompassPath() {
    if (!m_effects.isActive(EffectType::CompassTrail)) {
        m_compassPath.clear();
        return;
    }
    const bool stale = m_compassPath.empty() || m_compassPath.front() != m_player.cell();
    if (stale) {
        m_compassPath =
            shortestPath(m_maze, m_player.cell(), m_maze.exit(), PassRules{true, false});
    }
}

void Level::advanceTimer(float deltaSeconds) {
    if (m_timerStarted && !m_effects.isActive(EffectType::TimeFreeze)) {
        m_elapsed += deltaSeconds;
    }
}

void Level::updateVisibility() {
    const int radius = m_spec.hasFog ? fogRadius() : CLEAR_EXPLORE_RADIUS;
    if (m_player.cell() == m_lastRevealCell && radius == m_lastRevealRadius) {
        return;
    }
    m_fog.reveal(m_maze, m_player.cell(), radius);
    if (!m_spec.hasFog) {
        m_fog.revealCorridors(m_maze, m_player.cell(), CORRIDOR_SIGHT_CELLS);
    }
    m_lastRevealCell = m_player.cell();
    m_lastRevealRadius = radius;
}

void Level::updateChaser(float deltaSeconds) {
    if (!m_spec.hasChaser || !m_timerStarted) {
        return;
    }
    if (!m_chaser.isActive()) {
        m_graceRemaining = std::max(0.0f, m_graceRemaining - deltaSeconds);
        if (m_graceRemaining <= 0.0f) {
            m_chaser.activate(Chaser::chooseSpawnCell(m_maze, m_player.cell(), m_spawnRng));
            emit(LevelEventType::ChaserSpawned, m_chaser.cell());
        }
        return;
    }
    const float modeFactor = m_mode == ViewMode::FirstPerson3D ? CHASER_SPEED_3D_FACTOR : 1.0f;
    const float speed =
        Chaser::speedFactor(m_spec.chaserBaseSpeed, m_chaser.activeSeconds()) * modeFactor;
    const bool frozen = m_effects.isActive(EffectType::ChaserFreeze);
    m_chaser.advance(deltaSeconds, m_maze, m_player.cell(), moveSecondsFor(m_mode) / speed, frozen);
    m_chaserDistance =
        core::shortestPathLength(m_maze, m_chaser.cell(), m_player.cell(), PassRules{});
}

void Level::checkCaught() {
    if (!m_chaser.isActive()) {
        return;
    }
    const bool sameCell =
        m_chaser.cell() == m_player.cell() && !m_chaser.isMoving() && !m_player.isMoving();
    const float gap = distanceBetween(m_chaser.visualPosition(), m_player.visualPosition());
    if (sameCell || gap < CHASER_CATCH_DISTANCE) {
        m_state = LevelState::Caught;
        emit(LevelEventType::Caught, m_player.cell());
    }
}

void Level::win() {
    m_state = LevelState::Won;
    m_earnedStars = starsForTime(m_elapsed, m_parTime);
    emit(LevelEventType::Won, m_player.cell());
}

void Level::emit(LevelEventType type, Cell cell, PerkType perk) {
    m_events.push_back(LevelEvent{type, cell, perk});
}

LevelSnapshot Level::snapshot() const {
    LevelSnapshot result;
    result.level = m_spec.level;
    result.seed = m_seed;
    result.playerCell = m_player.cell();
    result.facing = m_player.facing();
    result.elapsedMs = toMilliseconds(m_elapsed);
    result.timerStarted = m_timerStarted;
    result.berries = m_player.berries();
    for (const EffectType type : ALL_EFFECT_TYPES) {
        result.effectMs[static_cast<std::size_t>(type)] = toMilliseconds(m_effects.remaining(type));
    }
    for (const PerkItem& perk : m_perks) {
        if (perk.collected) {
            result.collectedPerkIds.push_back(perk.id);
        }
    }
    result.exploredHex = m_fog.encodeExplored();
    result.chaserActive = m_chaser.isActive();
    result.chaserCell = m_chaser.isActive() ? m_chaser.cell() : Cell{};
    result.chaserActiveMs = toMilliseconds(m_chaser.activeSeconds());
    result.graceMs = toMilliseconds(m_graceRemaining);
    return result;
}

bool Level::restorePerksAndEffects(const LevelSnapshot& saved) {
    for (const int id : saved.collectedPerkIds) {
        if (id < 0 || id >= static_cast<int>(m_perks.size())) {
            return false;
        }
    }
    for (PerkItem& perk : m_perks) {
        perk.collected = std::find(saved.collectedPerkIds.begin(), saved.collectedPerkIds.end(),
                                   perk.id) != saved.collectedPerkIds.end();
    }
    m_effects.clear();
    for (const EffectType type : ALL_EFFECT_TYPES) {
        m_effects.setRemaining(
            type, millisecondsToSeconds(saved.effectMs[static_cast<std::size_t>(type)]));
    }
    return true;
}

bool Level::restore(const LevelSnapshot& saved) {
    const bool matches = saved.level == m_spec.level && saved.seed == m_seed;
    const bool positionsValid = m_maze.inBounds(saved.playerCell) &&
                                (!saved.chaserActive || m_maze.inBounds(saved.chaserCell));
    const bool chaserAllowed = !saved.chaserActive || m_spec.hasChaser;
    if (!matches || !positionsValid || !chaserAllowed || saved.elapsedMs < 0 || isFinished()) {
        return false;
    }
    Fog restoredFog(m_maze.size());
    if (!saved.exploredHex.empty() && !restoredFog.decodeExplored(saved.exploredHex)) {
        return false;
    }
    if (!restorePerksAndEffects(saved)) { // validates every id before changing anything
        return false;
    }
    m_fog = std::move(restoredFog);
    m_player.placeAt(saved.playerCell);
    m_player.setFacing(saved.facing);
    m_player.setBerries(saved.berries);
    m_elapsed = millisecondsToSeconds(saved.elapsedMs);
    m_timerStarted = saved.timerStarted;
    m_bufferedMove.reset();
    m_graceRemaining = millisecondsToSeconds(saved.graceMs);
    m_chaser = Chaser();
    if (saved.chaserActive) {
        m_chaser.activate(saved.chaserCell, millisecondsToSeconds(saved.chaserActiveMs));
    }
    m_lastRevealCell = Cell{-1, -1};
    updateVisibility();
    refreshCompassPath();
    return true;
}

} // namespace core
