#include "render/WallRuns.h"

#include <algorithm>

namespace render {

namespace {

bool isHorizontalWall(const core::Maze& maze, int x, int lineY) {
    const int size = maze.size();
    const core::Cell cell{x, std::min(lineY, size - 1)};
    const core::Direction side = lineY < size ? core::Direction::North : core::Direction::South;
    return maze.hasWall(cell, side) && !maze.hasSecretDoor(cell, side);
}

bool isVerticalWall(const core::Maze& maze, int lineX, int y) {
    const int size = maze.size();
    const core::Cell cell{std::min(lineX, size - 1), y};
    const core::Direction side = lineX < size ? core::Direction::West : core::Direction::East;
    return maze.hasWall(cell, side) && !maze.hasSecretDoor(cell, side);
}

} // namespace

std::vector<WallRun> computeWallRuns(const core::Maze& maze) {
    std::vector<WallRun> runs;
    const int size = maze.size();
    for (int line = 0; line <= size; ++line) {
        int horizontalStart = -1;
        int verticalStart = -1;
        for (int along = 0; along <= size; ++along) {
            const bool horizontal = along < size && isHorizontalWall(maze, along, line);
            const bool vertical = along < size && isVerticalWall(maze, line, along);
            if (horizontal && horizontalStart < 0) {
                horizontalStart = along;
            }
            if (!horizontal && horizontalStart >= 0) {
                runs.push_back(
                    WallRun{Vector2{static_cast<float>(horizontalStart), static_cast<float>(line)},
                            Vector2{static_cast<float>(along), static_cast<float>(line)}});
                horizontalStart = -1;
            }
            if (vertical && verticalStart < 0) {
                verticalStart = along;
            }
            if (!vertical && verticalStart >= 0) {
                runs.push_back(
                    WallRun{Vector2{static_cast<float>(line), static_cast<float>(verticalStart)},
                            Vector2{static_cast<float>(line), static_cast<float>(along)}});
                verticalStart = -1;
            }
        }
    }
    return runs;
}

} // namespace render
