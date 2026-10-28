#include "TestHarness.h"
#include "TestUtil.h"

#include "core/MazeGenerator.h"
#include "core/TextView.h"

#include <string>
#include <vector>

using namespace core;

namespace {

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines(1);
    for (const char character : text) {
        if (character == '\n') {
            lines.emplace_back();
        } else {
            lines.back().push_back(character);
        }
    }
    return lines;
}

char charAt(const std::vector<std::string>& lines, int column, int row) {
    return lines[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
}

} // namespace

TEST_CASE(textview, outputHasTwoNPlusOneRowsAndColumns) {
    for (const int size : {10, 23, 50}) {
        Rng rng(static_cast<std::uint32_t>(size));
        const Maze maze = generatePerfectMaze(size, rng);
        const std::vector<std::string> lines = splitLines(mazeToText(maze));
        CHECK_EQ(static_cast<int>(lines.size()), 2 * size + 1);
        for (const std::string& line : lines) {
            CHECK_EQ(static_cast<int>(line.size()), 2 * size + 1);
        }
    }
}

TEST_CASE(textview, startAndExitAreMarked) {
    Rng rng(4U);
    const Maze maze = generatePerfectMaze(10, rng);
    const std::vector<std::string> lines = splitLines(mazeToText(maze));
    CHECK_EQ(charAt(lines, 1, 1), TEXT_START);
    CHECK_EQ(charAt(lines, 19, 19), TEXT_EXIT);
}

TEST_CASE(textview, wallCharactersMatchMazeLayout) {
    Rng rng(8U);
    const Maze maze = generatePerfectMaze(12, rng);
    const std::vector<std::string> lines = splitLines(mazeToText(maze));
    for (int y = 0; y < 12; ++y) {
        for (int x = 0; x + 1 < 12; ++x) {
            const char expected = maze.isOpen(Cell{x, y}, Direction::East) ? TEXT_OPEN : TEXT_WALL;
            CHECK_EQ(charAt(lines, 2 * x + 2, 2 * y + 1), expected);
        }
    }
    CHECK_EQ(charAt(lines, 0, 0), TEXT_WALL);
}

TEST_CASE(textview, secretDoorsAndMarkersAreDrawn) {
    Maze maze(10);
    REQUIRE(maze.setSecretDoor(Cell{2, 2}, Direction::East));
    TextViewOptions options;
    options.markers.push_back(TextMarker{Cell{5, 6}, TEXT_PLAYER});
    options.markers.push_back(TextMarker{Cell{1, 8}, TEXT_PERK});
    const std::vector<std::string> lines = splitLines(mazeToText(maze, options));
    CHECK_EQ(charAt(lines, 6, 5), TEXT_SECRET_DOOR);
    CHECK_EQ(charAt(lines, 11, 13), TEXT_PLAYER);
    CHECK_EQ(charAt(lines, 3, 17), TEXT_PERK);
}

TEST_CASE(textview, unexploredCellsAreHidden) {
    const Maze maze = testutil::openMaze(10);
    Grid2D<bool> explored(10, 10, false);
    explored[0][0] = true;
    TextViewOptions options;
    options.explored = &explored;
    const std::vector<std::string> lines = splitLines(mazeToText(maze, options));
    CHECK_EQ(charAt(lines, 1, 1), TEXT_START);
    CHECK_EQ(charAt(lines, 2, 1), TEXT_OPEN);      // wall slot next to an explored cell
    CHECK_EQ(charAt(lines, 5, 5), TEXT_UNKNOWN);   // interior of cell (2,2)
    CHECK_EQ(charAt(lines, 19, 19), TEXT_UNKNOWN); // the exit has not been seen yet
}
