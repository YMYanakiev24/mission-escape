#include "core/MazeGenerator.h"

#include "core/Config.h"
#include "core/Pathfinding.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace core {

DfsMazeBuilder::DfsMazeBuilder(int size, Rng& rng)
    : m_maze(size), m_visited(size, size, false), m_rng(rng.nextU32()) {
    if (size > 0) {
        const Cell start = m_maze.start();
        m_visited[start.y][start.x] = true;
        m_stack.push_back(start);
    }
}

bool DfsMazeBuilder::step() {
    if (m_stack.empty()) {
        return false;
    }
    const Cell current = m_stack.back();
    std::array<Direction, DIRECTION_COUNT> candidates{};
    int candidateCount = 0;
    for (const Direction direction : ALL_DIRECTIONS) {
        const Cell next = neighbour(current, direction);
        if (m_maze.inBounds(next) && !m_visited[next.y][next.x]) {
            candidates[static_cast<std::size_t>(candidateCount++)] = direction;
        }
    }
    if (candidateCount == 0) {
        m_stack.pop_back(); // dead end: backtrack
        return !m_stack.empty();
    }
    const Direction chosen =
        candidates[static_cast<std::size_t>(m_rng.nextInt(0, candidateCount - 1))];
    const Cell next = neighbour(current, chosen);
    m_maze.carvePassage(current, chosen);
    m_visited[next.y][next.x] = true;
    m_stack.push_back(next);
    return true;
}

Maze generatePerfectMaze(int size, Rng& rng) {
    DfsMazeBuilder builder(size, rng);
    while (builder.step()) {
    }
    return builder.takeMaze();
}

namespace {

constexpr int NO_PARENT = -1;
constexpr int DECOY_CHOICE_POOL = 3;

/// The spanning tree of a perfect maze, rooted at the start cell.
struct RootedTree {
    Grid2D<int> parentDirection; ///< Direction (as int) from a cell to its parent.
    Grid2D<int> subtreeSize;     ///< Number of cells in the subtree below each cell.
    Grid2D<bool> onSolutionPath;
    std::vector<Cell> order;        ///< Breadth-first order from the root.
    std::vector<Cell> solutionPath; ///< Start ... exit.
};

Cell parentOf(const RootedTree& tree, Cell cell) {
    return neighbour(cell, static_cast<Direction>(tree.parentDirection[cell.y][cell.x]));
}

void collectBreadthFirstOrder(const Maze& maze, RootedTree& tree) {
    Grid2D<bool> visited(maze.size(), maze.size(), false);
    visited[0][0] = true;
    tree.order.push_back(maze.start());
    for (std::size_t head = 0; head < tree.order.size(); ++head) {
        const Cell cell = tree.order[head];
        for (const Direction direction : ALL_DIRECTIONS) {
            const Cell next = neighbour(cell, direction);
            if (maze.isOpen(cell, direction) && !visited[next.y][next.x]) {
                visited[next.y][next.x] = true;
                tree.parentDirection[next.y][next.x] = static_cast<int>(opposite(direction));
                tree.order.push_back(next);
            }
        }
    }
}

RootedTree buildRootedTree(const Maze& maze) {
    const int size = maze.size();
    RootedTree tree{Grid2D<int>(size, size, NO_PARENT),
                    Grid2D<int>(size, size, 1),
                    Grid2D<bool>(size, size, false),
                    {},
                    {}};
    collectBreadthFirstOrder(maze, tree);
    // Children come after their parents in BFS order, so a reverse sweep sums subtrees.
    for (std::size_t index = tree.order.size(); index-- > 1;) {
        const Cell cell = tree.order[index];
        const Cell parent = parentOf(tree, cell);
        tree.subtreeSize[parent.y][parent.x] += tree.subtreeSize[cell.y][cell.x];
    }
    for (Cell cell = maze.exit(); tree.parentDirection[cell.y][cell.x] != NO_PARENT;
         cell = parentOf(tree, cell)) {
        tree.onSolutionPath[cell.y][cell.x] = true;
        tree.solutionPath.insert(tree.solutionPath.begin(), cell);
    }
    tree.onSolutionPath[0][0] = true;
    tree.solutionPath.insert(tree.solutionPath.begin(), maze.start());
    return tree;
}

/// A whole side branch that touches @p pathCell through the wall on @p side and could be
/// re-hung there. @p branchRoot is where the branch currently joins the solution path.
struct DecoyCandidate {
    Cell pathCell;
    Direction side = Direction::North;
    Cell branchRoot;
    int branchSize = 0;
};

/// Climbs from an off-path cell to the first cell of its branch (whose parent is on the path).
Cell branchRootOf(const RootedTree& tree, Cell cell) {
    Cell parent = parentOf(tree, cell);
    while (!tree.onSolutionPath[parent.y][parent.x]) {
        cell = parent;
        parent = parentOf(tree, cell);
    }
    return cell;
}

std::vector<DecoyCandidate> findDecoyCandidates(const Maze& maze, const RootedTree& tree,
                                                int window) {
    std::vector<DecoyCandidate> candidates;
    const int pathLength = static_cast<int>(tree.solutionPath.size());
    const int first = std::max(1, pathLength - 1 - window);
    for (int index = first; index < pathLength - 1; ++index) { // never the exit itself
        const Cell pathCell = tree.solutionPath[static_cast<std::size_t>(index)];
        for (const Direction side : ALL_DIRECTIONS) {
            if (maze.isBoundary(pathCell, side) || !maze.hasWall(pathCell, side)) {
                continue;
            }
            const Cell touching = neighbour(pathCell, side);
            if (tree.onSolutionPath[touching.y][touching.x]) {
                continue;
            }
            const Cell root = branchRootOf(tree, touching);
            const int branchSize = tree.subtreeSize[root.y][root.x];
            const bool alreadyHere = parentOf(tree, root) == pathCell;
            if (!alreadyHere && branchSize >= EXIT_DECOY_MIN_BRANCH_CELLS) {
                candidates.push_back(DecoyCandidate{pathCell, side, root, branchSize});
            }
        }
    }
    return candidates;
}

bool addOneExitDecoy(Maze& maze, Rng& rng) {
    const RootedTree tree = buildRootedTree(maze);
    const int pathLength = static_cast<int>(tree.solutionPath.size());
    // Look at the last stretch of the path first and widen the window only if it offers no
    // suitable branch, so decoys end up as close to the exit as the layout allows.
    std::vector<DecoyCandidate> candidates;
    for (int window = EXIT_DECOY_PATH_WINDOW; candidates.empty() && window < 2 * pathLength;
         window *= 2) {
        candidates = findDecoyCandidates(maze, tree, window);
    }
    if (candidates.empty()) {
        return false;
    }
    // stable_sort keeps equal candidates in scan order, so the result is portable.
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const DecoyCandidate& lhs, const DecoyCandidate& rhs) {
                         return lhs.branchSize > rhs.branchSize;
                     });
    const int pool = std::min(DECOY_CHOICE_POOL, static_cast<int>(candidates.size()));
    const DecoyCandidate chosen = candidates[static_cast<std::size_t>(rng.nextInt(0, pool - 1))];
    const Cell root = chosen.branchRoot;
    // Cut the branch where it joins the path, then hang it on the path cell next to the exit.
    // Exactly one edge is removed and one added between the two resulting components, so the
    // maze stays a spanning tree and the solution path (which neither edge touches) is kept.
    maze.addWall(root, static_cast<Direction>(tree.parentDirection[root.y][root.x]));
    maze.carvePassage(chosen.pathCell, chosen.side);
    return true;
}

/// An inner wall that would shorten the start-to-exit path if it became a door.
struct DoorCandidate {
    Cell cell;
    Direction side = Direction::East;
};

std::vector<DoorCandidate> findDoorCandidates(const Maze& maze) {
    std::vector<DoorCandidate> candidates;
    const Grid2D<int> fromStart = bfsDistances(maze, maze.start(), PassRules{});
    const Grid2D<int> toExit = bfsDistances(maze, maze.exit(), PassRules{});
    const int baseLength = fromStart[maze.exit().y][maze.exit().x];
    if (baseLength == UNREACHABLE) {
        return candidates;
    }
    for (int y = 0; y < maze.size(); ++y) {
        for (int x = 0; x < maze.size(); ++x) {
            const Cell a{x, y};
            for (const Direction side : {Direction::East, Direction::South}) {
                if (maze.isBoundary(a, side) || !maze.hasWall(a, side) ||
                    maze.hasSecretDoor(a, side)) {
                    continue;
                }
                const Cell b = neighbour(a, side);
                const int through = std::min(fromStart[a.y][a.x] + 1 + toExit[b.y][b.x],
                                             fromStart[b.y][b.x] + 1 + toExit[a.y][a.x]);
                if (baseLength - through >= SECRET_DOOR_MIN_SAVING) {
                    candidates.push_back(DoorCandidate{a, side});
                }
            }
        }
    }
    return candidates;
}

} // namespace

int candidateScore(const Maze& maze) {
    const int pathLength = shortestPathLength(maze, maze.start(), maze.exit(), PassRules{});
    return pathLength * CANDIDATE_PATH_WEIGHT + maze.deadEndCount() * CANDIDATE_DEAD_END_WEIGHT;
}

Maze generateHardestCandidate(int size, int candidateCount, Rng& rng) {
    Maze best = generatePerfectMaze(size, rng);
    int bestScore = candidateScore(best);
    for (int candidate = 1; candidate < candidateCount; ++candidate) {
        Maze maze = generatePerfectMaze(size, rng);
        const int score = candidateScore(maze);
        if (score > bestScore) {
            best = std::move(maze);
            bestScore = score;
        }
    }
    return best;
}

int addExitDecoys(Maze& maze, int decoyCount, Rng& rng) {
    int added = 0;
    while (added < decoyCount && addOneExitDecoy(maze, rng)) {
        ++added;
    }
    return added;
}

int braidMaze(Maze& maze, float ratio, Rng& rng) {
    std::vector<Cell> deadEnds;
    for (int y = 0; y < maze.size(); ++y) {
        for (int x = 0; x < maze.size(); ++x) {
            if (maze.isDeadEnd(Cell{x, y})) {
                deadEnds.push_back(Cell{x, y});
            }
        }
    }
    rng.shuffle(deadEnds);
    const int target = static_cast<int>(
        std::lround(static_cast<float>(deadEnds.size()) * std::clamp(ratio, 0.0f, 1.0f)));
    int opened = 0;
    for (const Cell cell : deadEnds) {
        if (opened >= target) {
            break;
        }
        if (!maze.isDeadEnd(cell)) {
            continue; // a neighbour's new opening already fixed this one
        }
        std::vector<Direction> walls;
        std::vector<Direction> towardDeadEnds;
        for (const Direction direction : ALL_DIRECTIONS) {
            if (!maze.isBoundary(cell, direction) && maze.hasWall(cell, direction)) {
                walls.push_back(direction);
                if (maze.isDeadEnd(neighbour(cell, direction))) {
                    towardDeadEnds.push_back(direction); // removes two dead ends at once
                }
            }
        }
        const std::vector<Direction>& pool = towardDeadEnds.empty() ? walls : towardDeadEnds;
        if (pool.empty()) {
            continue;
        }
        maze.carvePassage(
            cell,
            pool[static_cast<std::size_t>(rng.nextInt(0, static_cast<int>(pool.size()) - 1))]);
        ++opened;
    }
    return opened;
}

std::vector<SecretDoor> placeSecretDoors(Maze& maze, int count, Rng& rng) {
    std::vector<DoorCandidate> candidates = findDoorCandidates(maze);
    rng.shuffle(candidates);
    Grid2D<bool> used(maze.size(), maze.size(), false);
    std::vector<SecretDoor> doors;
    for (const DoorCandidate& candidate : candidates) {
        if (static_cast<int>(doors.size()) >= count) {
            break;
        }
        const Cell other = neighbour(candidate.cell, candidate.side);
        if (used[candidate.cell.y][candidate.cell.x] || used[other.y][other.x]) {
            continue;
        }
        if (maze.setSecretDoor(candidate.cell, candidate.side)) {
            used[candidate.cell.y][candidate.cell.x] = true;
            used[other.y][other.x] = true;
            doors.push_back(SecretDoor{candidate.cell, candidate.side});
        }
    }
    return doors;
}

GeneratedMaze generateLevelMaze(const LevelSpec& spec, int size, Rng& rng) {
    GeneratedMaze result;
    result.maze = spec.candidateCount > 1 ? generateHardestCandidate(size, spec.candidateCount, rng)
                                          : generatePerfectMaze(size, rng);
    if (spec.exitDecoys) {
        addExitDecoys(result.maze, EXIT_DECOY_COUNT, rng);
    }
    if (spec.braidRatio > 0.0f) {
        braidMaze(result.maze, spec.braidRatio, rng);
    }
    if (spec.secretDoorMax > 0) {
        const int doorCount = rng.nextInt(spec.secretDoorMin, spec.secretDoorMax);
        result.secretDoors = placeSecretDoors(result.maze, doorCount, rng);
    }
    return result;
}

} // namespace core
