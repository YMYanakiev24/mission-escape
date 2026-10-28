#include "TestHarness.h"
#include "TestUtil.h"

#include "core/Chaser.h"
#include "core/Config.h"
#include "core/Level.h"
#include "core/MazeGenerator.h"
#include "core/Pathfinding.h"

#include <algorithm>

using namespace core;

namespace {

constexpr float FRAME = 1.0f / 60.0f;

} // namespace

TEST_CASE(chaser, everyStepReducesDistanceToPlayer) {
    Rng rng(81U);
    Maze maze = generatePerfectMaze(25, rng);
    braidMaze(maze, BRAID_RATIO, rng);
    const Cell player{3, 20};
    Chaser chaser;
    chaser.activate(Cell{24, 0});
    int distance = shortestPathLength(maze, chaser.cell(), player, PassRules{});
    REQUIRE(distance > 0);
    Cell previous = chaser.cell();
    for (int frame = 0; frame < 100000 && chaser.cell() != player; ++frame) {
        chaser.advance(FRAME, maze, player, 0.2f, false);
        if (chaser.cell() != previous) { // a new step has just started
            const int next = shortestPathLength(maze, chaser.cell(), player, PassRules{});
            CHECK_EQ(next, distance - 1);
            distance = next;
            previous = chaser.cell();
        }
    }
    CHECK_EQ(distance, 0);
}

TEST_CASE(chaser, neverUsesSecretDoors) {
    Maze maze(10);
    // A corridor along row 0 and a secret door that would be a shortcut to the player.
    for (int x = 0; x < 9; ++x) {
        maze.carvePassage(Cell{x, 0}, Direction::East);
    }
    REQUIRE(maze.setSecretDoor(Cell{0, 0}, Direction::South));
    Chaser chaser;
    chaser.activate(Cell{0, 0});
    for (int frame = 0; frame < 600; ++frame) {
        chaser.advance(FRAME, maze, Cell{0, 1}, 0.2f, false);
    }
    CHECK_EQ(chaser.cell(), (Cell{0, 0})); // the door is the only way and it stays shut
}

TEST_CASE(chaser, frozenChaserDoesNotMove) {
    const Maze maze = testutil::openMaze(12);
    Chaser chaser;
    chaser.activate(Cell{11, 11});
    for (int frame = 0; frame < 300; ++frame) {
        chaser.advance(FRAME, maze, Cell{0, 0}, 0.2f, true);
    }
    CHECK_EQ(chaser.cell(), (Cell{11, 11}));
    CHECK(!chaser.isMoving());
}

TEST_CASE(chaser, spawnsAtLeastHalfTheMazeAwayAndNotNearTheExit) {
    for (const std::uint32_t seed : {1U, 9U, 99U}) {
        Rng rng(seed);
        Maze maze = generatePerfectMaze(30, rng);
        braidMaze(maze, BRAID_RATIO, rng);
        for (int sample = 0; sample < 10; ++sample) {
            const Cell spawn = Chaser::chooseSpawnCell(maze, maze.start(), rng);
            CHECK(shortestPathLength(maze, maze.start(), spawn, PassRules{}) >= 15);
            CHECK(shortestPathLength(maze, maze.exit(), spawn, PassRules{}) >=
                  CHASER_EXIT_CLEARANCE);
        }
    }
}

TEST_CASE(chaser, speedRampsUpButIsCapped) {
    CHECK_EQ(Chaser::speedFactor(0.7f, 0.0f), 0.7f);
    CHECK(Chaser::speedFactor(0.7f, 10.0f) > 0.7f);
    CHECK_EQ(Chaser::speedFactor(0.85f, 100000.0f), CHASER_SPEED_MAX);
}

TEST_CASE(chaser, appearsAfterGracePeriodAndCatchesAnIdlePlayer) {
    Level level(16, 15, 1234U, ViewMode::Classic2D);
    REQUIRE(level.spec().hasChaser);
    level.update(10.0f); // no grace countdown before the first move
    CHECK(!level.chaser().isActive());
    const auto step = nextStepToward(level.maze(), level.player().cell(), level.maze().exit(),
                                     PassRules{true, false});
    REQUIRE(step.has_value());
    level.tryMove(*step);
    for (float time = 0.0f; time < CHASER_GRACE_SECONDS - 0.5f; time += FRAME) {
        level.update(FRAME);
    }
    CHECK(!level.chaser().isActive());
    for (float time = 0.0f; time < 1.0f; time += FRAME) {
        level.update(FRAME);
    }
    CHECK(level.chaser().isActive());
    for (int frame = 0; frame < 60 * 120 && !level.isFinished(); ++frame) {
        level.update(FRAME); // the player stands still
    }
    CHECK(level.state() == LevelState::Caught);
    CHECK(level.tryMove(Direction::North) == MoveResult::Inactive);
}

TEST_CASE(chaser, dangerRisesAsTheChaserApproaches) {
    Level level(17, 12, 55U, ViewMode::Classic2D);
    CHECK_EQ(level.chaserDanger(), 0.0f);
    const auto step = nextStepToward(level.maze(), level.player().cell(), level.maze().exit(),
                                     PassRules{true, false});
    REQUIRE(step.has_value());
    level.tryMove(*step);
    float highest = 0.0f;
    for (int frame = 0; frame < 60 * 60 && !level.isFinished(); ++frame) {
        level.update(FRAME);
        highest = std::max(highest, level.chaserDanger());
    }
    CHECK(highest > 0.5f);
}
