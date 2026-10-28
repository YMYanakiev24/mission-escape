#include "TestHarness.h"
#include "TestUtil.h"

#include "core/Config.h"
#include "core/Level.h"
#include "core/MazeGenerator.h"
#include "core/Pathfinding.h"
#include "core/Perks.h"

#include <algorithm>
#include <optional>

using namespace core;

namespace {

constexpr float FRAME = 1.0f / 60.0f;

void finishMove(Level& level) {
    for (int frame = 0; frame < 600 && level.player().isMoving(); ++frame) {
        level.update(FRAME);
    }
}

/// Walks the shortest path (doors allowed) to @p target; returns false if it got stuck.
bool walkTo(Level& level, Cell target) {
    for (int guard = 0; guard < 10000 && level.player().cell() != target; ++guard) {
        const auto step =
            nextStepToward(level.maze(), level.player().cell(), target, PassRules{true, false});
        if (!step || level.tryMove(*step) != MoveResult::Moved) {
            return false;
        }
        finishMove(level);
        if (level.isFinished()) {
            return level.player().cell() == target;
        }
    }
    return level.player().cell() == target;
}

/// The first perk of @p type in @p level, if any.
std::optional<PerkItem> findPerk(const Level& level, PerkType type) {
    for (const PerkItem& perk : level.perks()) {
        if (perk.type == type) {
            return perk;
        }
    }
    return std::nullopt;
}

/// Builds levels with increasing seeds until one contains a perk of @p type.
std::optional<Level> levelWithPerk(int levelNumber, PerkType type) {
    for (std::uint32_t seed = 1; seed < 400; ++seed) {
        Level level(levelNumber, 16, seed, ViewMode::Classic2D);
        const std::optional<PerkItem> perk = findPerk(level, type);
        if (!perk) {
            continue;
        }
        // Skip layouts where the walk to the perk would cross the exit (and end the level).
        const std::vector<Cell> route =
            shortestPath(level.maze(), level.maze().start(), perk->cell, PassRules{true, false});
        if (std::find(route.begin(), route.end(), level.maze().exit()) == route.end()) {
            return level;
        }
    }
    return std::nullopt;
}

bool hasEvent(const std::vector<LevelEvent>& events, LevelEventType type) {
    return std::any_of(events.begin(), events.end(),
                       [type](const LevelEvent& event) { return event.type == type; });
}

} // namespace

TEST_CASE(perks, spawnRulesKeepStartAreaAndExitFree) {
    for (const Tier tier : {Tier::Awakening, Tier::Labyrinth, Tier::Fog, Tier::Hunted}) {
        for (const std::uint32_t seed : {1U, 2U, 3U}) {
            Rng rng(seed);
            const Maze maze = generatePerfectMaze(MAX_MAZE_SIZE, rng);
            const std::vector<PerkItem> perks = spawnPerks(maze, tier, rng);
            CHECK_EQ(static_cast<int>(perks.size()), perkCountFor(MAX_MAZE_SIZE, tier));
            for (std::size_t index = 0; index < perks.size(); ++index) {
                const PerkItem& perk = perks[index];
                CHECK(manhattanDistance(perk.cell, maze.start()) > 1);
                CHECK(perk.cell != maze.exit());
                CHECK(isPerkAllowedInTier(perk.type, tier));
                CHECK_EQ(perk.id, static_cast<int>(index));
                for (std::size_t other = index + 1; other < perks.size(); ++other) {
                    CHECK(perks[other].cell != perk.cell);
                }
            }
            CHECK(perks.front().type == PerkType::MagicBerry);
        }
    }
}

TEST_CASE(perks, perkCountScalesWithSizeAndTier) {
    CHECK_EQ(perkCountFor(10, Tier::Awakening), 1 + PERK_BONUS_AWAKENING);
    CHECK_EQ(perkCountFor(48, Tier::Awakening), 6 + PERK_BONUS_AWAKENING);
    CHECK(perkCountFor(30, Tier::Fog) > perkCountFor(30, Tier::Awakening));
}

TEST_CASE(perks, tierSpecificPerksOnlyAppearInTheirTiers) {
    CHECK(!isPerkAllowedInTier(PerkType::CompassSpark, Tier::Awakening));
    CHECK(isPerkAllowedInTier(PerkType::CompassSpark, Tier::Hunted));
    CHECK(isPerkAllowedInTier(PerkType::Lantern, Tier::Fog));
    CHECK(!isPerkAllowedInTier(PerkType::Lantern, Tier::Labyrinth));
    CHECK(isPerkAllowedInTier(PerkType::FrostShard, Tier::Hunted));
    CHECK(!isPerkAllowedInTier(PerkType::FrostShard, Tier::Fog));
}

TEST_CASE(perks, magicBerryIsStoredAndPhasesThroughInnerWalls) {
    std::optional<Level> level = levelWithPerk(2, PerkType::MagicBerry);
    REQUIRE(level.has_value());
    const PerkItem berry = *findPerk(*level, PerkType::MagicBerry);
    REQUIRE(walkTo(*level, berry.cell));
    const int carried = level->player().berries(); // the walk may pass other berries too
    CHECK(carried >= 1);
    level->takeEvents();
    REQUIRE(level->activateBerry());
    CHECK(level->isPhasing());
    CHECK_EQ(level->player().berries(), carried - 1);
    // Some wall around the player is now passable, but the boundary never is.
    const Cell here = level->player().cell();
    for (const Direction direction : ALL_DIRECTIONS) {
        if (level->maze().isBoundary(here, direction)) {
            CHECK(level->tryMove(direction) == MoveResult::Blocked);
        }
    }
    CHECK(!level->activateBerry()); // already phasing
    for (float time = 0.0f; time < PHASE_SECONDS + 0.1f; time += FRAME) {
        level->update(FRAME);
    }
    CHECK(!level->isPhasing());
    LevelSnapshot empty = level->snapshot();
    empty.berries = 0;
    REQUIRE(level->restore(empty));
    level->takeEvents();
    CHECK(!level->activateBerry()); // no berries left
    CHECK(hasEvent(level->takeEvents(), LevelEventType::NoBerries));
}

TEST_CASE(perks, phaseNeverCrossesTheBoundaryAndEndsOnAValidCell) {
    Level level(1, 10, 3U, ViewMode::Classic2D);
    LevelSnapshot snapshot = level.snapshot();
    snapshot.berries = 1;
    REQUIRE(level.restore(snapshot));
    REQUIRE(level.activateBerry());
    CHECK(level.tryMove(Direction::North) == MoveResult::Blocked);
    CHECK(level.tryMove(Direction::West) == MoveResult::Blocked);
    // Walk east through whatever walls are there while the phase lasts.
    for (int step = 0; step < 30 && level.isPhasing(); ++step) {
        if (level.tryMove(Direction::East) != MoveResult::Moved) {
            level.tryMove(Direction::South);
        }
        finishMove(level);
    }
    finishMove(level);
    CHECK(level.maze().inBounds(level.player().cell()));
    CHECK(!level.player().isMoving());
}

TEST_CASE(perks, fullInventoryLeavesBerryOnTheFloor) {
    std::optional<Level> level = levelWithPerk(1, PerkType::MagicBerry);
    REQUIRE(level.has_value());
    LevelSnapshot snapshot = level->snapshot();
    snapshot.berries = BERRY_CAPACITY;
    REQUIRE(level->restore(snapshot));
    const PerkItem berry = *findPerk(*level, PerkType::MagicBerry);
    REQUIRE(walkTo(*level, berry.cell));
    CHECK_EQ(level->player().berries(), BERRY_CAPACITY);
    CHECK(hasEvent(level->takeEvents(), LevelEventType::InventoryFull));
    CHECK(!findPerk(*level, PerkType::MagicBerry)->collected);
}

TEST_CASE(perks, hourglassFreezesTheTimer) {
    std::optional<Level> level = levelWithPerk(3, PerkType::Hourglass);
    REQUIRE(level.has_value());
    REQUIRE(walkTo(*level, findPerk(*level, PerkType::Hourglass)->cell));
    REQUIRE(level->effects().isActive(EffectType::TimeFreeze));
    const float frozenAt = level->elapsedSeconds();
    for (float time = 0.0f; time < HOURGLASS_SECONDS - 0.2f; time += FRAME) {
        level->update(FRAME);
    }
    CHECK_EQ(level->elapsedSeconds(), frozenAt);
    for (float time = 0.0f; time < 1.0f; time += FRAME) {
        level->update(FRAME);
    }
    CHECK(level->elapsedSeconds() > frozenAt);
}

TEST_CASE(perks, compassSparkShowsTheShortestPathToTheExit) {
    std::optional<Level> level = levelWithPerk(7, PerkType::CompassSpark);
    REQUIRE(level.has_value());
    REQUIRE(walkTo(*level, findPerk(*level, PerkType::CompassSpark)->cell));
    const std::vector<Cell>& trail = level->compassPath();
    REQUIRE(!trail.empty());
    CHECK_EQ(trail.front(), level->player().cell());
    CHECK_EQ(trail.back(), level->maze().exit());
    CHECK_EQ(static_cast<int>(trail.size()) - 1,
             shortestPathLength(level->maze(), level->player().cell(), level->maze().exit(),
                                PassRules{true, false}));
    for (float time = 0.0f; time < COMPASS_SECONDS + 0.1f; time += FRAME) {
        level->update(FRAME);
    }
    CHECK(level->compassPath().empty());
}

TEST_CASE(perks, lanternDoublesTheFogRadius) {
    std::optional<Level> level = levelWithPerk(13, PerkType::Lantern);
    REQUIRE(level.has_value());
    CHECK_EQ(level->fogRadius(), FOG_RADIUS);
    REQUIRE(walkTo(*level, findPerk(*level, PerkType::Lantern)->cell));
    CHECK_EQ(level->fogRadius(), FOG_RADIUS * LANTERN_RADIUS_MULTIPLIER);
}

TEST_CASE(perks, frostShardFreezesTheChaser) {
    std::optional<Level> level = levelWithPerk(18, PerkType::FrostShard);
    REQUIRE(level.has_value());
    REQUIRE(walkTo(*level, findPerk(*level, PerkType::FrostShard)->cell));
    if (level->isFinished()) {
        return; // caught on the way: nothing left to check for this seed
    }
    CHECK(level->effects().isActive(EffectType::ChaserFreeze));
    if (level->chaser().isActive()) {
        const Cell before = level->chaser().cell();
        level->update(FRAME * 30.0f);
        CHECK_EQ(level->chaser().cell(), before);
    }
}

TEST_CASE(perks, collectedPerksAndEffectsSurviveSnapshots) {
    std::optional<Level> level = levelWithPerk(3, PerkType::Hourglass);
    REQUIRE(level.has_value());
    const PerkItem hourglass = *findPerk(*level, PerkType::Hourglass);
    REQUIRE(walkTo(*level, hourglass.cell));
    const LevelSnapshot snapshot = level->snapshot();
    Level resumed(3, 16, level->seed(), ViewMode::Classic2D);
    REQUIRE(resumed.restore(snapshot));
    CHECK(findPerk(resumed, PerkType::Hourglass)->collected);
    CHECK(resumed.effects().isActive(EffectType::TimeFreeze));
    LevelSnapshot broken = snapshot;
    broken.collectedPerkIds.push_back(999);
    CHECK(!resumed.restore(broken));
}
