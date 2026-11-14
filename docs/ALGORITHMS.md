# Algorithms

All algorithms live in the business logic layer (`src/business/core`) and do not depend on
rendering. Tuning values are in `Config.h`.

## Maze representation

`Maze` keeps one byte per cell in a `Grid2D` (a dynamic 2D array with two allocations and
`grid[y][x]` access). Bits 0–3 are the walls North, East, South and West; bits 4–7 mark the same
walls as secret doors. A secret door keeps its wall bit, so code that does not know about doors
(the Hunter's BFS, the fog) sees it as a plain wall. `carvePassage` and `addWall` change a wall
on both sides, so neighbouring cells always agree. The start is (0, 0) and the exit (N−1, N−1).

## Generation: recursive backtracker (DFS)

`DfsMazeBuilder` is a randomised depth-first search with an **explicit stack**:

1. Every wall is in place; the start is marked visited and pushed on the stack.
2. From the cell on top of the stack, pick a random unvisited neighbour.
3. Remove the wall between them, mark the neighbour visited and push it.
4. If there is no unvisited neighbour, pop the cell (backtrack).
5. Stop when the stack is empty.

The result is a **perfect maze**: a spanning tree with N² − 1 passages and exactly one path
between any two cells. The explicit stack avoids a stack overflow at 50 × 50 and lets the
generation be animated one step at a time (the menu background). Complexity: O(N²).

## Tier rules

| Tier | What changes |
|---|---|
| Awakening | One perfect maze |
| Labyrinth | 8 candidates, the hardest is kept (path length × 3 + dead ends); 3 exit decoys; 1–3 secret doors |
| Fog | 1–3 secret doors; fog of war with radius 3 |
| Hunted | 40% of dead ends opened (loops); 1–3 secret doors; the Hunter |

**Exit decoys.** The maze is treated as a tree rooted at the start (BFS: parents, subtree sizes,
path to the exit). A side branch of at least 4 cells that touches one of the last 12 path cells
through a wall is cut where it joins the path and re-attached next to the exit. Removing one
edge and adding one edge keeps the tree, so the maze stays perfect and the path to the exit does
not change.

**Braiding.** Dead ends are shuffled; from each one that is still a dead end one wall is removed,
preferably one that leads into another dead end, which fixes two at once.

**Secret doors.** BFS from the start and from the exit gives the distances dStart and dExit. An
inner wall between a and b is a candidate if the path through it, min(dStart[a] + 1 + dExit[b],
dStart[b] + 1 + dExit[a]), is at least 4 steps shorter than the normal shortest path. 1–3
candidates become doors, never two in the same cell.

## Determinism

`core::Rng` uses `std::mt19937`, whose raw output is fixed by the C++ standard. The standard
distributions may differ between compilers, so `nextInt` (rejection sampling, no modulo bias),
the shuffle (Fisher–Yates) and `nextFloat` are written by hand, sorting is stable, and directions
are always visited in N, E, S, W order. Each level's seed is mixed with the level number and the
maze size (`deriveLevelSeed`, SplitMix64). That is why a saved game stores only the seed and
regenerates the same maze.

## Pathfinding: BFS

`bfsDistances` returns the distance from one cell to every other cell (−1 when unreachable). The
queue is a vector with a moving head index. `shortestPath` runs BFS from the target and walks
"downhill" from the start to a neighbour whose distance is one less. Every step costs the same,
so BFS gives the shortest path in O(N²). It is used for the par time, the Labyrinth candidates,
exit decoys, secret doors, perk placement, the Hunter and the Compass Spark trail.

## Fog of war

`Fog` stores the visible and the explored cells. `reveal` is a depth-limited BFS (radius 3, or
6 with a Lantern) through open passages only, so you never see through walls. Explored cells are
remembered and saved as hexadecimal text (4 cells per digit). In tiers without fog, radius 2
plus straight-corridor sight of up to 10 cells fills the 3D minimap.

## The Hunter

In the Hunted tier the Hunter appears 3 s after the first move, on a random cell at least N/2
steps from the player and at least 4 from the exit. Whenever it stands still it runs a fresh BFS
to the player and takes one step. It never uses secret doors or phases through walls. Its speed
is 0.70–0.85 of the player's (by level), grows by 0.002 per second up to 0.95, and is × 0.9 in
3D. The player is caught when both stand on the same cell or are less than 0.55 cells apart. A
Frost Shard freezes it for 4 s.

## Scoring

- Par time = shortest path × 0.30 s × tier factor (1.0 / 0.85 / 1.5 / 1.0) × view factor
  (2D 1.0, 3D 1.75).
- 3 stars at ≤ 2 × par, 2 stars at ≤ 3.5 × par, otherwise 1. Quitting or being caught gives 0.
- The timer starts with the first move and stops for an Hourglass and while paused.
- Records keep the most stars and the best time separately; a 0-star attempt never changes them.

## Saving data

- `key=value` format: keys of letters, digits, "." and "_"; strict parsing – a line without "=",
  an invalid value or a repeated key makes the file invalid.
- Atomic write: the text goes to a temporary file that is then renamed over the real one.
- Full validation on load: version, name, size, records, skins, positions, effects, fog. The
  first error marks the slot "Corrupt"; the game never crashes on a bad file.
- An unfinished level is stored as a `LevelSnapshot` (position, time, perks, fog, Hunter). The
  maze itself is not stored – it is regenerated from the seed – and restoring checks everything
  before changing anything.

## Text view

`mazeToText` turns the maze into (2N + 1) × (2N + 1) characters: cell (x, y) is at (2x + 1,
2y + 1); `#` is a wall, `%` a secret door, `@` the player, `X` the Hunter, `*` a perk and `.` an
unexplored cell.
