#pragma once

#include "core/Cell.h"
#include "core/Grid2D.h"
#include "core/LevelRules.h"
#include "core/Maze.h"
#include "core/Random.h"

#include <utility>
#include <vector>

/// @file MazeGenerator.h
/// Random maze generation: iterative recursive backtracker (DFS with an explicit stack).

namespace core {

/// Step-by-step recursive backtracker. It uses an explicit stack instead of recursion, so a
/// 50 x 50 maze can never overflow the call stack, and it can be advanced one carve at a time
/// (the main menu background animates exactly this algorithm).
class DfsMazeBuilder {
public:
    /// Prepares a fully walled @p size x @p size maze and pushes the start cell.
    /// The builder owns a private generator seeded from @p rng, so results are deterministic.
    DfsMazeBuilder(int size, Rng& rng);

    /// Performs one carve (or one backtrack). Returns false once the maze is complete.
    bool step();

    /// True when every cell has been visited and the stack is empty.
    bool finished() const noexcept { return m_stack.empty(); }

    /// The maze in its current (possibly partial) state.
    const Maze& maze() const noexcept { return m_maze; }

    /// Cells visited so far.
    const Grid2D<bool>& visited() const noexcept { return m_visited; }

    /// The current DFS stack, bottom first (used for visualisation).
    const std::vector<Cell>& stack() const noexcept { return m_stack; }

    /// Moves the finished maze out of the builder.
    Maze takeMaze() { return std::move(m_maze); }

private:
    Maze m_maze;
    Grid2D<bool> m_visited;
    std::vector<Cell> m_stack;
    Rng m_rng;
};

/// Generates a perfect maze (exactly one path between any two cells, N*N-1 passages).
Maze generatePerfectMaze(int size, Rng& rng);

/// Location of a secret door: the wall on @p side of @p cell.
struct SecretDoor {
    Cell cell;
    Direction side = Direction::East;
};

/// A level's maze plus the secret doors placed in it.
struct GeneratedMaze {
    Maze maze;
    std::vector<SecretDoor> secretDoors;
};

/// Difficulty score of a candidate: longer solution paths and more dead ends score higher.
int candidateScore(const Maze& maze);

/// Generates @p candidateCount perfect mazes and keeps the one with the highest score.
Maze generateHardestCandidate(int size, int candidateCount, Rng& rng);

/// Re-hangs whole dead-end branches onto the last stretch of the solution path, so wrong turns
/// branch off right next to the exit. Each change is a tree-preserving edge swap: the maze
/// stays perfect and the start-to-exit path is unchanged. Returns the number of swaps made.
int addExitDecoys(Maze& maze, int decoyCount, Rng& rng);

/// Opens about @p ratio of all dead ends into loops (a "braided" maze); returns how many
/// dead ends were opened.
int braidMaze(Maze& maze, float ratio, Rng& rng);

/// Turns up to @p count inner walls into secret doors. A wall is only chosen if walking
/// through it shortens the door-free shortest path from start to exit by at least
/// SECRET_DOOR_MIN_SAVING steps (checked with BFS). Doors never share a cell.
std::vector<SecretDoor> placeSecretDoors(Maze& maze, int count, Rng& rng);

/// Builds the maze for a level according to its tier rules (candidates, decoys, braiding,
/// secret doors). Fully determined by the state of @p rng.
GeneratedMaze generateLevelMaze(const LevelSpec& spec, int size, Rng& rng);

} // namespace core
