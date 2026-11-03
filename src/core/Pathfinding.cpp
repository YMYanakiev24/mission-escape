#include "core/Pathfinding.h"

#include <cstddef>

namespace core {

Grid2D<int> bfsDistances(const Maze& maze, Cell source, const PassRules& rules) {
    Grid2D<int> distances(maze.size(), maze.size(), UNREACHABLE);
    if (!maze.inBounds(source)) {
        return distances;
    }
    // A vector with a moving head index is a queue that never reallocates more than N*N times.
    std::vector<Cell> queue;
    queue.reserve(static_cast<std::size_t>(maze.size()) * static_cast<std::size_t>(maze.size()));
    distances[source.y][source.x] = 0;
    queue.push_back(source);
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const Cell current = queue[head];
        const int nextDistance = distances[current.y][current.x] + 1;
        for (const Direction direction : ALL_DIRECTIONS) {
            if (!maze.canPass(current, direction, rules)) {
                continue;
            }
            const Cell next = neighbour(current, direction);
            if (distances[next.y][next.x] == UNREACHABLE) {
                distances[next.y][next.x] = nextDistance;
                queue.push_back(next);
            }
        }
    }
    return distances;
}

int shortestPathLength(const Maze& maze, Cell from, Cell to, const PassRules& rules) {
    if (!maze.inBounds(from) || !maze.inBounds(to)) {
        return UNREACHABLE;
    }
    return bfsDistances(maze, from, rules).at(to.x, to.y);
}

std::vector<Cell> pathFromDistanceMap(const Maze& maze, const Grid2D<int>& distancesToTarget,
                                      Cell from, const PassRules& rules) {
    std::vector<Cell> path;
    if (!maze.inBounds(from) || distancesToTarget.at(from.x, from.y) == UNREACHABLE) {
        return path;
    }
    Cell current = from;
    path.push_back(current);
    while (distancesToTarget[current.y][current.x] > 0) {
        const int wanted = distancesToTarget[current.y][current.x] - 1;
        for (const Direction direction : ALL_DIRECTIONS) {
            const Cell next = neighbour(current, direction);
            if (maze.canPass(current, direction, rules) &&
                distancesToTarget[next.y][next.x] == wanted) {
                current = next;
                break;
            }
        }
        path.push_back(current);
    }
    return path;
}

std::vector<Cell> shortestPath(const Maze& maze, Cell from, Cell to, const PassRules& rules) {
    if (!maze.inBounds(from) || !maze.inBounds(to)) {
        return {};
    }
    return pathFromDistanceMap(maze, bfsDistances(maze, to, rules), from, rules);
}

std::optional<Direction> nextStepToward(const Maze& maze, Cell from, Cell target,
                                        const PassRules& rules) {
    if (!maze.inBounds(from) || !maze.inBounds(target) || from == target) {
        return std::nullopt;
    }
    const Grid2D<int> distances = bfsDistances(maze, target, rules);
    const int current = distances[from.y][from.x];
    if (current == UNREACHABLE) {
        return std::nullopt;
    }
    for (const Direction direction : ALL_DIRECTIONS) {
        const Cell next = neighbour(from, direction);
        if (maze.canPass(from, direction, rules) && distances[next.y][next.x] == current - 1) {
            return direction;
        }
    }
    return std::nullopt;
}

bool allCellsReachable(const Maze& maze, const PassRules& rules) {
    const Grid2D<int> distances = bfsDistances(maze, maze.start(), rules);
    for (int y = 0; y < maze.size(); ++y) {
        for (int x = 0; x < maze.size(); ++x) {
            if (distances[y][x] == UNREACHABLE) {
                return false;
            }
        }
    }
    return true;
}

} // namespace core
