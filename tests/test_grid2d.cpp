#include "TestHarness.h"

#include "core/Grid2D.h"

#include <stdexcept>
#include <utility>

using core::Grid2D;

TEST_CASE(grid, constructorAllocatesRequestedSizeAndFillsValue) {
    const Grid2D<int> grid(7, 3, 42);
    CHECK_EQ(grid.width(), 7);
    CHECK_EQ(grid.height(), 3);
    CHECK(!grid.empty());
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            CHECK_EQ(grid[y][x], 42);
        }
    }
}

TEST_CASE(grid, defaultConstructedGridIsEmpty) {
    const Grid2D<int> grid;
    CHECK(grid.empty());
    CHECK_EQ(grid.width(), 0);
    CHECK_EQ(grid.height(), 0);
    CHECK(!grid.inBounds(0, 0));
}

TEST_CASE(grid, negativeDimensionsThrowInvalidArgument) {
    bool threw = false;
    try {
        const Grid2D<int> grid(-1, 5);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

TEST_CASE(grid, atThrowsOutOfRangeOutsideBounds) {
    Grid2D<int> grid(4, 4, 0);
    int throwCount = 0;
    const int badCoordinates[][2] = {{-1, 0}, {0, -1}, {4, 0}, {0, 4}};
    for (const auto& coordinate : badCoordinates) {
        try {
            grid.at(coordinate[0], coordinate[1]) = 1;
        } catch (const std::out_of_range&) {
            ++throwCount;
        }
    }
    CHECK_EQ(throwCount, 4);
}

TEST_CASE(grid, rowPointersAddressTheSameElementsAsAt) {
    Grid2D<int> grid(5, 4, 0);
    grid[2][3] = 17;
    CHECK_EQ(grid.at(3, 2), 17);
    grid.at(1, 3) = 9;
    CHECK_EQ(grid[3][1], 9);
}

TEST_CASE(grid, copyIsDeepAndIndependent) {
    Grid2D<int> original(3, 3, 1);
    Grid2D<int> copy(original);
    copy[1][1] = 5;
    CHECK_EQ(original[1][1], 1);
    CHECK_EQ(copy[1][1], 5);

    Grid2D<int> assigned(1, 1, 0);
    assigned = original;
    original[0][0] = 8;
    CHECK_EQ(assigned[0][0], 1);
    CHECK_EQ(assigned.width(), 3);
}

TEST_CASE(grid, selfAssignmentKeepsContents) {
    Grid2D<int> grid(2, 2, 3);
    Grid2D<int>& alias = grid;
    grid = alias;
    CHECK_EQ(grid[1][1], 3);
    CHECK_EQ(grid.width(), 2);
}

TEST_CASE(grid, moveTransfersOwnershipAndEmptiesSource) {
    Grid2D<int> source(6, 2, 4);
    Grid2D<int> moved(std::move(source));
    CHECK_EQ(moved.width(), 6);
    CHECK_EQ(moved[1][5], 4);
    CHECK(source.empty()); // NOLINT(bugprone-use-after-move): verifying moved-from state

    Grid2D<int> target(1, 1, 0);
    target = std::move(moved);
    CHECK_EQ(target.width(), 6);
    CHECK(moved.empty()); // NOLINT(bugprone-use-after-move)
}

TEST_CASE(grid, equalityComparesDimensionsAndValues) {
    Grid2D<int> a(3, 2, 1);
    Grid2D<int> b(3, 2, 1);
    CHECK(a == b);
    b[0][2] = 0;
    CHECK(a != b);
    const Grid2D<int> c(2, 3, 1);
    CHECK(a != c);
}

TEST_CASE(grid, worksWithBoolElements) {
    Grid2D<bool> flags(10, 10, false);
    flags[9][9] = true;
    CHECK(flags.at(9, 9));
    CHECK(!flags.at(0, 0));
    flags.fill(true);
    CHECK(flags.at(0, 0));
}
