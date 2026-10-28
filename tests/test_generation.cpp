#include "TestHarness.h"
#include "TestUtil.h"

#include "core/Config.h"
#include "core/MazeGenerator.h"
#include "core/Pathfinding.h"

#include <cstdint>

using namespace core;

namespace {

constexpr std::uint32_t SEEDS[] = {1U, 7U, 1234U, 99991U, 0xDEADBEEFU};
constexpr int SIZES[] = {MIN_MAZE_SIZE, 17, 33, MAX_MAZE_SIZE};

} // namespace

TEST_CASE(generation, perfectMazeReachesEveryCell) {
    for (const int size : SIZES) {
        for (const std::uint32_t seed : SEEDS) {
            Rng rng(seed);
            const Maze maze = generatePerfectMaze(size, rng);
            CHECK(allCellsReachable(maze, PassRules{}));
        }
    }
}

TEST_CASE(generation, perfectMazeHasExactlyNSquaredMinusOnePassages) {
    for (const int size : SIZES) {
        Rng rng(size * 31U);
        const Maze maze = generatePerfectMaze(size, rng);
        CHECK_EQ(maze.passageCount(), size * size - 1);
    }
}

TEST_CASE(generation, startIsTopLeftAndExitIsBottomRight) {
    for (const int size : SIZES) {
        Rng rng(5U);
        const Maze maze = generatePerfectMaze(size, rng);
        CHECK_EQ(maze.size(), size);
        CHECK_EQ(maze.start(), (Cell{0, 0}));
        CHECK_EQ(maze.exit(), (Cell{size - 1, size - 1}));
    }
}

TEST_CASE(generation, sameSeedProducesIdenticalMaze) {
    for (const std::uint32_t seed : SEEDS) {
        Rng first(seed);
        Rng second(seed);
        CHECK(generatePerfectMaze(25, first) == generatePerfectMaze(25, second));
    }
}

TEST_CASE(generation, differentSeedsProduceDifferentMazes) {
    Rng first(1U);
    Rng second(2U);
    CHECK(generatePerfectMaze(20, first) != generatePerfectMaze(20, second));
}

TEST_CASE(generation, boundaryWallsAreNeverCarved) {
    Rng rng(77U);
    const Maze maze = generatePerfectMaze(15, rng);
    for (int index = 0; index < maze.size(); ++index) {
        CHECK(maze.hasWall(Cell{index, 0}, Direction::North));
        CHECK(maze.hasWall(Cell{index, maze.size() - 1}, Direction::South));
        CHECK(maze.hasWall(Cell{0, index}, Direction::West));
        CHECK(maze.hasWall(Cell{maze.size() - 1, index}, Direction::East));
    }
}

TEST_CASE(generation, builderStepsVisitEveryCellBeforeFinishing) {
    Rng rng(3U);
    DfsMazeBuilder builder(12, rng);
    int steps = 0;
    while (builder.step()) {
        ++steps;
    }
    CHECK(builder.finished());
    for (int y = 0; y < 12; ++y) {
        for (int x = 0; x < 12; ++x) {
            CHECK(builder.visited()[y][x]);
        }
    }
    // Every carve pushes once and every cell is popped once: 2 * (N*N - 1) + 1 steps.
    CHECK_EQ(steps + 1, 2 * (12 * 12 - 1) + 1);
}

TEST_CASE(generation, wallsStayConsistentOnBothSides) {
    Rng rng(11U);
    const Maze maze = generatePerfectMaze(20, rng);
    for (int y = 0; y < maze.size(); ++y) {
        for (int x = 0; x + 1 < maze.size(); ++x) {
            CHECK_EQ(maze.hasWall(Cell{x, y}, Direction::East),
                     maze.hasWall(Cell{x + 1, y}, Direction::West));
            CHECK_EQ(maze.hasWall(Cell{y, x}, Direction::South),
                     maze.hasWall(Cell{y, x + 1}, Direction::North));
        }
    }
}

TEST_CASE(generation, derivedLevelSeedsDifferPerLevelAndSize) {
    const std::uint32_t base = 424242U;
    CHECK(deriveLevelSeed(base, 1, 20) != deriveLevelSeed(base, 2, 20));
    CHECK(deriveLevelSeed(base, 1, 20) != deriveLevelSeed(base, 1, 21));
    CHECK_EQ(deriveLevelSeed(base, 5, 30), deriveLevelSeed(base, 5, 30));
}

TEST_CASE(generation, rngNextIntStaysWithinInclusiveBounds) {
    Rng rng(9U);
    bool sawMinimum = false;
    bool sawMaximum = false;
    for (int draw = 0; draw < 2000; ++draw) {
        const int value = rng.nextInt(-3, 4);
        CHECK(value >= -3 && value <= 4);
        sawMinimum = sawMinimum || value == -3;
        sawMaximum = sawMaximum || value == 4;
    }
    CHECK(sawMinimum);
    CHECK(sawMaximum);
}
