#include "TestHarness.h"
#include "TestUtil.h"

#include "core/Maze.h"

using namespace core;

TEST_CASE(movement, wallsBlockNormalMovement) {
    const Maze maze(10);
    const PassRules normal{};
    for (const Direction direction : ALL_DIRECTIONS) {
        CHECK(!maze.canPass(Cell{5, 5}, direction, normal));
    }
}

TEST_CASE(movement, openPassagesAllowMovementBothWays) {
    Maze maze(10);
    maze.carvePassage(Cell{2, 2}, Direction::East);
    CHECK(maze.canPass(Cell{2, 2}, Direction::East, PassRules{}));
    CHECK(maze.canPass(Cell{3, 2}, Direction::West, PassRules{}));
    CHECK(!maze.canPass(Cell{2, 2}, Direction::South, PassRules{}));
}

TEST_CASE(movement, secretDoorsAllowOnlyMoversThatUseThem) {
    Maze maze(10);
    REQUIRE(maze.setSecretDoor(Cell{4, 4}, Direction::South));
    CHECK(maze.hasWall(Cell{4, 4}, Direction::South));
    CHECK(maze.hasSecretDoor(Cell{4, 5}, Direction::North));
    const PassRules player{true, false};
    const PassRules chaser{false, false};
    CHECK(maze.canPass(Cell{4, 4}, Direction::South, player));
    CHECK(maze.canPass(Cell{4, 5}, Direction::North, player));
    CHECK(!maze.canPass(Cell{4, 4}, Direction::South, chaser));
}

TEST_CASE(movement, secretDoorsCannotBePlacedOnBoundaryOrOpenings) {
    Maze maze(10);
    CHECK(!maze.setSecretDoor(Cell{0, 0}, Direction::North));
    CHECK(!maze.setSecretDoor(Cell{9, 3}, Direction::East));
    maze.carvePassage(Cell{1, 1}, Direction::East);
    CHECK(!maze.setSecretDoor(Cell{1, 1}, Direction::East));
}

TEST_CASE(movement, phaseAllowsInnerWallsButNeverTheBoundary) {
    const Maze maze(10);
    const PassRules phase{true, true};
    CHECK(maze.canPass(Cell{5, 5}, Direction::North, phase));
    CHECK(maze.canPass(Cell{5, 5}, Direction::West, phase));
    for (int index = 0; index < maze.size(); ++index) {
        CHECK(!maze.canPass(Cell{index, 0}, Direction::North, phase));
        CHECK(!maze.canPass(Cell{index, 9}, Direction::South, phase));
        CHECK(!maze.canPass(Cell{0, index}, Direction::West, phase));
        CHECK(!maze.canPass(Cell{9, index}, Direction::East, phase));
    }
}

TEST_CASE(movement, addWallRemovesSecretDoorFlags) {
    Maze maze(10);
    REQUIRE(maze.setSecretDoor(Cell{3, 3}, Direction::East));
    maze.addWall(Cell{3, 3}, Direction::East);
    CHECK(!maze.hasSecretDoor(Cell{3, 3}, Direction::East));
    CHECK(!maze.hasSecretDoor(Cell{4, 3}, Direction::West));
    CHECK(maze.hasWall(Cell{4, 3}, Direction::West));
}

TEST_CASE(movement, outOfBoundsCellsCannotMove) {
    const Maze maze = testutil::openMaze(10);
    CHECK(!maze.canPass(Cell{-1, 0}, Direction::East, PassRules{true, true}));
    CHECK(!maze.canPass(Cell{10, 10}, Direction::West, PassRules{true, true}));
}
