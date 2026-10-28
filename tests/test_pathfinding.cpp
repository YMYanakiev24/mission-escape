#include "TestHarness.h"
#include "TestUtil.h"

#include "core/MazeGenerator.h"
#include "core/Pathfinding.h"

#include <cstddef>

using namespace core;

namespace {

/// True when consecutive path cells are neighbours connected under @p rules.
bool isConnectedPath(const Maze& maze, const std::vector<Cell>& path, const PassRules& rules) {
    for (std::size_t index = 1; index < path.size(); ++index) {
        const Cell from = path[index - 1];
        const Cell to = path[index];
        bool connected = false;
        for (const Direction direction : ALL_DIRECTIONS) {
            connected = connected ||
                        (neighbour(from, direction) == to && maze.canPass(from, direction, rules));
        }
        if (!connected) {
            return false;
        }
    }
    return true;
}

} // namespace

TEST_CASE(pathfinding, bfsFindsKnownShortestPathInSerpentine) {
    const Maze maze = testutil::serpentineMaze(11);
    const std::vector<Cell> path = shortestPath(maze, maze.start(), maze.exit(), PassRules{});
    REQUIRE(!path.empty());
    CHECK_EQ(static_cast<int>(path.size()), 11 * 11); // visits every cell
    CHECK_EQ(path.front(), maze.start());
    CHECK_EQ(path.back(), maze.exit());
    CHECK(isConnectedPath(maze, path, PassRules{}));
}

TEST_CASE(pathfinding, bfsFindsManhattanPathInOpenField) {
    const Maze maze = testutil::openMaze(12);
    CHECK_EQ(shortestPathLength(maze, maze.start(), maze.exit(), PassRules{}), 2 * 11);
    CHECK_EQ(shortestPathLength(maze, Cell{3, 4}, Cell{7, 1}, PassRules{}), 4 + 3);
}

TEST_CASE(pathfinding, pathLengthMatchesDistanceMap) {
    Rng rng(2024U);
    const Maze maze = generatePerfectMaze(30, rng);
    const Grid2D<int> distances = bfsDistances(maze, maze.start(), PassRules{});
    const std::vector<Cell> path = shortestPath(maze, maze.start(), maze.exit(), PassRules{});
    REQUIRE(!path.empty());
    CHECK_EQ(static_cast<int>(path.size()) - 1, distances.at(maze.exit().x, maze.exit().y));
    CHECK(isConnectedPath(maze, path, PassRules{}));
}

TEST_CASE(pathfinding, unreachableTargetsGiveEmptyPath) {
    const Maze walled(10); // no passages at all
    CHECK(shortestPath(walled, walled.start(), walled.exit(), PassRules{}).empty());
    CHECK_EQ(shortestPathLength(walled, walled.start(), walled.exit(), PassRules{}), UNREACHABLE);
    CHECK(!nextStepToward(walled, walled.start(), walled.exit(), PassRules{}).has_value());
}

TEST_CASE(pathfinding, nextStepAlwaysReducesDistanceByOne) {
    Rng rng(555U);
    const Maze maze = generatePerfectMaze(25, rng);
    const Cell target{12, 20};
    const Grid2D<int> distances = bfsDistances(maze, target, PassRules{});
    Cell walker{24, 0};
    int guard = 0;
    while (walker != target && guard++ < 25 * 25) {
        const auto step = nextStepToward(maze, walker, target, PassRules{});
        REQUIRE(step.has_value());
        const Cell next = neighbour(walker, *step);
        CHECK_EQ(distances.at(next.x, next.y), distances.at(walker.x, walker.y) - 1);
        walker = next;
    }
    CHECK_EQ(walker, target);
}

TEST_CASE(pathfinding, pathToSelfIsSingleCell) {
    const Maze maze = testutil::openMaze(10);
    const std::vector<Cell> path = shortestPath(maze, Cell{4, 4}, Cell{4, 4}, PassRules{});
    REQUIRE(path.size() == 1U);
    CHECK_EQ(path.front(), (Cell{4, 4}));
}
