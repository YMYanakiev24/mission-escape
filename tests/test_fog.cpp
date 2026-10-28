#include "TestHarness.h"
#include "TestUtil.h"

#include "core/Config.h"
#include "core/Fog.h"
#include "core/Level.h"
#include "core/MazeGenerator.h"
#include "core/Pathfinding.h"

using namespace core;

TEST_CASE(fog, visibleCellsAreAlwaysWithinRadiusThroughOpenPassages) {
    for (const std::uint32_t seed : {1U, 2U, 3U, 4U}) {
        Rng rng(seed);
        const Maze maze = generatePerfectMaze(20, rng);
        Fog fog(20);
        for (int sample = 0; sample < 25; ++sample) {
            const Cell center{rng.nextInt(0, 19), rng.nextInt(0, 19)};
            fog.reveal(maze, center, FOG_RADIUS);
            const Grid2D<int> distances = bfsDistances(maze, center, PassRules{});
            for (int y = 0; y < 20; ++y) {
                for (int x = 0; x < 20; ++x) {
                    const bool closeEnough =
                        distances[y][x] != UNREACHABLE && distances[y][x] <= FOG_RADIUS;
                    CHECK_EQ(fog.isVisible(Cell{x, y}), closeEnough);
                }
            }
        }
    }
}

TEST_CASE(fog, wallsBlockSightEvenToAdjacentCells) {
    Maze maze(10);
    maze.carvePassage(Cell{0, 0}, Direction::South); // the only opening out of (0,0)
    Fog fog(10);
    fog.reveal(maze, Cell{0, 0}, FOG_RADIUS);
    CHECK(fog.isVisible(Cell{0, 0}));
    CHECK(fog.isVisible(Cell{0, 1}));
    CHECK(!fog.isVisible(Cell{1, 0})); // right next to us, but behind a wall
}

TEST_CASE(fog, secretDoorsBlockSight) {
    Maze maze(10);
    REQUIRE(maze.setSecretDoor(Cell{0, 0}, Direction::East));
    Fog fog(10);
    fog.reveal(maze, Cell{0, 0}, FOG_RADIUS);
    CHECK(!fog.isVisible(Cell{1, 0}));
}

TEST_CASE(fog, exploredCellsAreRememberedButNotVisible) {
    const Maze maze = testutil::openMaze(12);
    Fog fog(12);
    fog.reveal(maze, Cell{0, 0}, 1);
    fog.reveal(maze, Cell{11, 11}, 1);
    CHECK(fog.isExplored(Cell{0, 0}));
    CHECK(!fog.isVisible(Cell{0, 0}));
    CHECK(fog.isVisible(Cell{11, 11}));
    CHECK_EQ(fog.exploredCount(), 3 + 3);
}

TEST_CASE(fog, encodedExplorationRoundTrips) {
    for (const int size : {10, 13, 50}) {
        const Maze maze = testutil::openMaze(size);
        Fog fog(size);
        fog.reveal(maze, Cell{size / 2, size / 3}, 4);
        Fog copy(size);
        REQUIRE(copy.decodeExplored(fog.encodeExplored()));
        CHECK(copy.explored() == fog.explored());
    }
    Fog fog(10);
    CHECK(!fog.decodeExplored("zz"));
    CHECK(!fog.decodeExplored(std::string(26, 'g')));
}

TEST_CASE(fog, fogLevelsRevealOnlyNearThePlayer) {
    const Level level(12, 30, 404U, ViewMode::Classic2D);
    CHECK(level.spec().hasFog);
    CHECK(level.fog().isVisible(Cell{0, 0}));
    CHECK(!level.fog().isExplored(level.maze().exit()));
    CHECK(!level.isExitSeen());
    CHECK(level.fog().exploredCount() < 30 * 30 / 4);
}
