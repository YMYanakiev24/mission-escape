#include "TestHarness.h"
#include "TestUtil.h"

#include "core/Config.h"
#include "core/Level.h"
#include "core/Pathfinding.h"
#include "core/Scoring.h"

using namespace core;

namespace {

constexpr std::uint32_t SEED = 777U;
constexpr float FRAME = 1.0f / 60.0f;

/// Advances the level until the current move has finished.
void finishMove(Level& level) {
    for (int frame = 0; frame < 600 && level.player().isMoving(); ++frame) {
        level.update(FRAME);
    }
}

/// A direction from the player's cell that is blocked by a plain wall, if any.
std::optional<Direction> blockedDirection(const Level& level) {
    for (const Direction direction : ALL_DIRECTIONS) {
        if (!level.maze().canPass(level.player().cell(), direction, PassRules{true, false})) {
            return direction;
        }
    }
    return std::nullopt;
}

} // namespace

TEST_CASE(movement, levelStartsOnStartCellWithTimerStopped) {
    const Level level(1, 12, SEED, ViewMode::Classic2D);
    CHECK_EQ(level.player().cell(), (Cell{0, 0}));
    CHECK(!level.timerStarted());
    CHECK_EQ(level.state(), LevelState::Playing);
}

TEST_CASE(movement, wallsBlockPlayerMoves) {
    Level level(1, 12, SEED, ViewMode::Classic2D);
    const std::optional<Direction> blocked = blockedDirection(level);
    REQUIRE(blocked.has_value());
    CHECK(level.tryMove(*blocked) == MoveResult::Blocked);
    CHECK_EQ(level.player().cell(), (Cell{0, 0}));
    CHECK(!level.timerStarted()); // a bump does not start the timer
}

TEST_CASE(movement, timerStartsOnFirstSuccessfulMove) {
    Level level(1, 12, SEED, ViewMode::Classic2D);
    const auto step = nextStepToward(level.maze(), level.player().cell(), level.maze().exit(),
                                     PassRules{true, false});
    REQUIRE(step.has_value());
    level.update(1.0f); // waiting before moving does not count
    CHECK_EQ(level.elapsedSeconds(), 0.0f);
    CHECK(level.tryMove(*step) == MoveResult::Moved);
    CHECK(level.timerStarted());
    CHECK(level.tryMove(*step) == MoveResult::Busy); // still animating
    finishMove(level);
    CHECK(level.elapsedSeconds() > 0.0f);
}

TEST_CASE(movement, bufferedMoveRunsWhenCurrentMoveEnds) {
    Level level(1, 12, SEED, ViewMode::Classic2D);
    const std::vector<Cell> path = shortestPath(level.maze(), level.maze().start(),
                                                level.maze().exit(), PassRules{true, false});
    REQUIRE(path.size() >= 3U);
    const auto first = nextStepToward(level.maze(), path[0], path[1], PassRules{true, false});
    const auto second = nextStepToward(level.maze(), path[1], path[2], PassRules{true, false});
    REQUIRE(first.has_value() && second.has_value());
    level.tryMove(*first);
    level.bufferMove(*second);
    finishMove(level); // first move ends, buffered move starts
    finishMove(level);
    CHECK_EQ(level.player().cell(), path[2]);
}

TEST_CASE(movement, turningChangesFacingOnly) {
    Level level(1, 12, SEED, ViewMode::FirstPerson3D);
    const Direction before = level.player().facing();
    level.turnRight();
    CHECK(level.player().facing() == turnedRight(before));
    level.turnLeft();
    level.turnLeft();
    CHECK(level.player().facing() == turnedLeft(before));
    CHECK_EQ(level.player().cell(), (Cell{0, 0}));
}

TEST_CASE(playthrough, solverFollowingBfsPathCompletesLevelWithStars) {
    for (const int size : {MIN_MAZE_SIZE, 25, MAX_MAZE_SIZE}) {
        Level level(1, size, SEED + static_cast<std::uint32_t>(size), ViewMode::Classic2D);
        int guard = 0;
        while (!level.isFinished() && guard++ < size * size * 4) {
            const auto step = nextStepToward(level.maze(), level.player().cell(),
                                             level.maze().exit(), PassRules{true, false});
            REQUIRE(step.has_value());
            REQUIRE(level.tryMove(*step) == MoveResult::Moved);
            finishMove(level);
        }
        CHECK_EQ(level.state(), LevelState::Won);
        CHECK_EQ(level.player().cell(), level.maze().exit());
        // Walking the shortest path without detours is always well inside the 3-star time.
        CHECK_EQ(level.earnedStars(), 3);
        CHECK_EQ(level.earnedStars(), starsForTime(level.elapsedSeconds(), level.parTimeSeconds()));
        CHECK(level.tryMove(Direction::North) == MoveResult::Inactive);
    }
}

TEST_CASE(playthrough, slowPlayerEarnsFewerStars) {
    Level level(1, 10, SEED, ViewMode::Classic2D);
    const auto first = nextStepToward(level.maze(), level.player().cell(), level.maze().exit(),
                                      PassRules{true, false});
    REQUIRE(first.has_value());
    level.tryMove(*first);
    finishMove(level);
    // Dawdle past the two-star limit before walking to the exit.
    const float dawdle = starTimeLimit(2, level.parTimeSeconds()) + 1.0f;
    for (float waited = 0.0f; waited < dawdle; waited += 0.5f) {
        level.update(0.5f);
    }
    while (!level.isFinished()) {
        const auto step = nextStepToward(level.maze(), level.player().cell(), level.maze().exit(),
                                         PassRules{true, false});
        REQUIRE(step.has_value());
        level.tryMove(*step);
        finishMove(level);
    }
    CHECK_EQ(level.earnedStars(), 1);
}

TEST_CASE(generation, levelGenerationIsDeterministicForSeedLevelAndSize) {
    const Level first(7, 20, 4242U, ViewMode::Classic2D);
    const Level second(7, 20, 4242U, ViewMode::Classic2D);
    const Level otherLevel(8, 20, 4242U, ViewMode::Classic2D);
    CHECK(first.maze() == second.maze());
    CHECK(first.maze() != otherLevel.maze());
}
