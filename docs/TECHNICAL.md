# Mission Escape: Technical Documentation

## 1. Architecture

```
main.cpp  ─►  app::App  ── owns the window (RAII), the audio device and the main loop
                 │
                 ├─ app::ScreenStack   state machine: a stack of ui::Screen objects with fades
                 │      └─ ui::*Screen  one class per screen (menus, gameplay, results)
                 ├─ app::AppContext    shared services: theme, audio, settings, save slots,
                 │                     navigator, the running GameSession
                 └─ app::GameSession   gameplay controller
                        ├─ core::Level          the rules (pure C++)
                        ├─ render::Renderer2D / Renderer3D / Minimap / Hud / Effects
                        └─ app::FeedbackPresenter  core events → particles, toasts, sounds
```

| Layer | Folder | Depends on | Notes |
|---|---|---|---|
| Core | `src/core` | C++17 standard library only | No raylib, no `<iostream>`, no `printf`; fully unit-tested |
| Persistence | `src/io` | core, `<fstream>`, `<filesystem>` | Compiled into `maze_core`, tested headless |
| Rendering | `src/render` | core (read-only), raylib | Never changes game rules |
| UI | `src/ui` | render, app/Input | Widgets plus one class per screen |
| Audio | `src/audio` | raylib | `Synth` itself is pure C++ and has its own tests |
| Application | `src/app` | everything | Main loop, state machine, input mapping |

**Rule of the codebase:** input is translated into *commands* on `core::Level` (`tryMove`,
`bufferMove`, `turnLeft`, `activateBerry`). The core reports what happened as `LevelEvent`
values, and the presentation layer turns them into feedback. The renderers only *read*
`core::Level`.

### Main loop (`app::App::run`)

1. Clamp the frame time to 0.1 s, so a stalled window can never teleport anything.
2. Recompute the UI scale (layout is designed for 1280 × 720 and scales uniformly).
3. Poll input once into an `InputState` (the screenshot tour may replace it with scripted input).
4. `ScreenStack::update`: the top screen updates. Navigation requests fade to black, swap
   screens on the black frame (applying deferred session changes), then fade back in.
5. Audio: apply volumes, choose the music track (menu or the tier of the running level),
   stream music.
6. Draw the visible screens (overlays draw the screen below first), then the fade.

Every animation uses the frame's delta time; the target is 60 FPS.

## 2. Data structures

### 2.1 `core::Grid2D<T>`: dynamic 2D array

All per-cell maps (walls, fog memory, BFS distances, visited flags, decoy trees, 2D fog fade,
text-view canvas) are `Grid2D` instances. **No per-cell map uses `std::vector` or
`std::array`.**

```
m_rows ──► [ row 0 ptr ][ row 1 ptr ] ... [ row h-1 ptr ]     new T*[h]
               │            │
               ▼            ▼
m_storage ─► [ x0 x1 ... xw-1 | x0 x1 ... xw-1 | ... ]          new T[w*h]
```

- **Allocation:** one contiguous block `new T[w * h]` holds the elements, plus a table of row
  pointers `new T*[h]` pointing into it. This gives the classic `grid[y][x]` syntax and good
  cache locality, with only two allocations.
- **Exception safety:** if the second allocation throws, the first is freed before
  re-throwing. Constructors that throw after allocating release the memory themselves,
  because a destructor does not run for a constructor that never finished.
- **Rule of five:** the copy constructor deep-copies. Copy assignment uses copy-and-swap
  (strong guarantee). Move construction and move assignment steal both pointers and leave the
  source empty. The destructor `delete[]`s both arrays.
- **Access:** `at(x, y)` is bounds-checked and throws `std::out_of_range`; `operator[]`
  returns a row pointer for fast unchecked loops.
- **Why not `std::vector<std::vector<T>>`:** the assignment requires dynamic memory managed by
  hand, and one contiguous block is faster than `h` separate vectors.

### 2.2 `core::Maze`: wall bitmask

Each cell is one `uint8_t`:

| Bit | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Meaning | wall N | wall E | wall S | wall W | secret N | secret E | secret S | secret W |

- Walls are kept consistent on both sides: carving the east wall of a cell also carves the west
  wall of its neighbour.
- A secret door keeps its wall bit **and** sets its secret bit. Everything that ignores secret
  doors (the chaser, fog visibility, the door-free BFS) therefore sees a wall automatically.
- `canPass(cell, direction, PassRules)` decides movement:
  - the boundary is always closed;
  - an open passage is always open;
  - a wall is passable only while phasing, or if it is a secret door and the mover uses doors.

### 2.3 `core::Level`: one attempt

`Level` owns:
- the `Maze` and its list of `SecretDoor`s;
- the `PerkItem`s;
- the `Player` (a `Mover` with facing and berries);
- the `Fog`;
- the `Chaser` (another `Mover`);
- the `EffectTimers`;
- the timer, the state (Playing / Won / Caught) and an event queue.

`update(dt)` advances time, effects, the player's tween (arrival collects perks and checks the
exit), visibility, the chaser, and finally the catch test. The whole simulation is
deterministic for a given seed and sequence of commands.

### 2.4 Save data

- `core::SaveData`: name, size, view mode, `Progress`, and an optional `LevelSnapshot`.
- `core::LevelSnapshot`: level, seed, player cell and facing, elapsed time in milliseconds,
  timer flag, berries, the remaining time of every effect, collected perk ids, explored fog as
  hexadecimal text (4 cells per digit), chaser state and remaining grace time.

The maze itself is never stored. It is **regenerated from the seed**, which is why generation
must be deterministic.

## 3. Algorithms

Mermaid flowcharts of the main algorithms are in [ALGORITHMS.md](ALGORITHMS.md).

### 3.1 Portable randomness

`std::mt19937` output is defined by the standard, but `std::uniform_int_distribution` and
`std::shuffle` are not. `core::Rng` therefore implements unbiased bounded integers (rejection
sampling) and Fisher–Yates itself, so a seed gives the same maze with MSVC, GCC and Clang. The
seed for a level is `deriveLevelSeed(seed, level, size)`, a SplitMix64 mix.

### 3.2 Maze generation: iterative recursive backtracker

`DfsMazeBuilder` keeps an explicit stack, starting with the start cell marked visited. Each
step looks at the top cell:
- if it has unvisited neighbours, pick one at random, carve the wall to it, mark it visited
  and push it;
- otherwise pop.

The maze is finished when the stack is empty. The result is a perfect maze with exactly N² − 1
passages. Being iterative, it cannot overflow the call stack on 50 × 50, and it can be advanced
one step per frame: the main menu background animates exactly this algorithm.

### 3.3 Tier 2: candidate selection and exit decoys

- **Candidate selection:** 8 perfect mazes are generated. Each gets the score
  `3 × shortestPathLength + deadEnds`, and the best one is kept.
- **Decoys:** the maze is turned into a tree rooted at the start (BFS parents, subtree sizes,
  solution path).
  - For a cell near the end of the solution path, look at an off-path neighbour behind a wall.
    Its whole side branch (climbing up to where it joins the path) is a candidate.
  - Among the largest branches, one is cut where it joins the path and re-attached to the cell
    next to the exit. One edge is removed and one added between the two resulting components,
    so the maze stays a spanning tree (still perfect) and the solution path is unchanged.
  - If the last 12 path cells offer no branch, the search window doubles.

### 3.4 Tier 4: braiding

Collect the dead ends and shuffle them. Open about 40% of them, preferring a wall towards
another dead end, which removes two at once. This creates loops, so the player can evade the
chaser.

### 3.5 Secret doors

1. Compute BFS distances from the start and from the exit, ignoring doors.
2. For every inner wall (a, b), the route through it has length
   `min(dS(a) + 1 + dE(b), dS(b) + 1 + dE(a))`.
3. The wall is a candidate if this saves at least 4 steps against the door-free shortest path.
4. Shuffle the candidates and place 1–3 doors that do not share a cell.

The tests verify that each door, on its own, shortens the path.

### 3.6 BFS

`bfsDistances` fills a `Grid2D<int>` using a vector as a queue. `shortestPath` walks from the
start to a neighbour with distance − 1, in the fixed order N, E, S, W, so results are
deterministic.

### 3.7 Fog of war

`Fog::reveal` is a depth-limited BFS flood fill through **open passages only**, up to radius 3
(6 with the Lantern). Visible cells are added to the explored memory. Because it follows
passages, the player never sees through walls or secret doors. Outside the fog tier the same
code, plus straight-corridor sight lines, records the explored cells shown on the 3D minimap.

### 3.8 The chaser

- After a 3 s grace period (counted from the first move), `chooseSpawnCell` picks a random cell
  at least N/2 path steps from the player and at least 4 from the exit.
- Whenever it stands still, the chaser runs a fresh BFS from the player and steps to a
  neighbour that is one step closer. It never uses secret doors.
- Step duration = `playerStepDuration / speed`, where speed starts at the tier base
  (0.70 → 0.85) and ramps up by 0.002 per second, capped at 0.95.
- The player is caught when the interpolated distance between the two is below 0.55 cells,
  which also covers swapping cells mid-step.

### 3.9 Stars

`parTime = shortestPathLength × 0.30 × tierMultiplier × modeMultiplier`.
3★ ≤ 2.0 × par, 2★ ≤ 3.5 × par, otherwise 1★. A quit scores 0 and never changes the record.

## 4. Rendering

- **2D:**
  - The static walls are merged into straight runs (`WallRuns`) and rendered **once per level
    and cell size** into a render texture. Three opaque glow layers are each drawn to a scratch
    target and composited with low opacity using a premultiplied-alpha blend mode, so
    overlapping runs never double-blend; a crisp line and a bright core go on top.
  - Each frame only blits that texture and draws the animated parts: secret door runes, the
    portal, perks, the compass trail, the player skin, the chaser, the fog overlay and the exit
    beacon.
  - If every cell fits at ≥ 24 px the whole maze is shown; otherwise a `Camera2D` follows the
    player and is clamped to the maze.
- **3D:**
  - All walls form **one mesh**: one box per wall run, each face split into a floor strip, a
    gradient body and a glowing top band, textured with a per-cell panel tile.
  - The floor and ceiling are textured planes. An embedded GLSL 330 shader adds linear
    distance fog; if it fails to compile, the default shader is used instead.
  - Immediate-mode shapes (portal, perks, chaser, doors) are fogged on the CPU.
  - Turns are interpolated; the head bob is optional.
- **UI:** retained widgets (`Button`, `Slider`, `Toggle`, `TextField`) inside a `FocusGroup`
  that moves the focus spatially with the arrow keys and follows the mouse. All colours come
  from `render::palette`.
- **Resources:** `RenderTarget`, `TextureResource`, `ModelResource` and `ShaderResource` are
  move-only RAII owners. `App` declares the window guard first, so these are always destroyed
  while the OpenGL context still exists.

## 5. Audio

`audio::Synth` builds every sound from sine waves, band-limited squares, noise, one-pole
low-pass filters and envelopes, then normalises them. The music loops are 16 s pads (four
chords with smooth overlapping windows, a drone and sparkles). Every frequency is rounded to a
whole number of cycles per loop, so the loop is seamless; a unit test checks the seam.
`AudioManager` turns the samples into raylib `Sound`s, streams music through an `AudioStream`
with 1.6 s crossfades, honours the volume settings, and uses `assets/audio/<name>.ogg` instead
when such a file exists.

## 6. Persistence

- Text files of `key=value` lines starting with a `version=` header.
- Writes go to `file.tmp`, which is then renamed over the target. A crash therefore never leaves
  a half-written save, and the folder is created on demand.
- The parser is strict: malformed lines, duplicate keys, out-of-range numbers, unknown skins,
  an invalid name, inconsistent progress (a completed level whose successor is locked) or a
  snapshot that does not fit the maze all mark the slot as **corrupted**. The game shows that
  in Load Game and never crashes.
- Settings are parsed leniently: unknown keys are ignored, bad values fall back to defaults,
  and numbers are clamped.

---

## Backend: `core/` and `io/`

- Owns the rules and data: `Grid2D`, `Maze`, `MazeGenerator`, `Pathfinding`, `Level`,
  `Player`/`Mover`, `Chaser`, `Fog`, `Perks`, `Scoring`, `LevelRules`, `Progress`, `Settings`,
  `Validation`, `TextView`, `Random`, plus the `io` save and settings stores.
- Constraints: no raylib, no standard input/output, every tuning value in `Config.h`.
- Extension points:
  - a new perk = enum value + text in `Perks.cpp` + effect in `Level::applyPerk`;
  - a new tier rule = a field in `LevelSpec` + a step in `generateLevelMaze`.

## Frontend: `render/`, `ui/` and `audio/`

- `render`: palette, theme and text, 2D and 3D renderers, minimap, HUD, perk icons, skins,
  effects, the text view, the menu background.
- `ui`: widgets, `FocusGroup`, `ConfirmDialog`, `TabBar` and one class per screen.
- `audio`: synthesiser and manager.
- Rule: renderers read `core::Level` but never change it; screens talk to the core only through
  `GameSession` / `AppContext`.

## QA: `tests/`

- `TestHarness.h` is a self-written harness, about 100 lines, with no dependencies:
  - `TEST_CASE(suite, name)` registers a function in a static registry;
  - `CHECK` records a failure and continues;
  - `CHECK_EQ` prints both values;
  - `REQUIRE` aborts the current test.
- `test_main.cpp` runs everything or `--suite <name>`, and `--list` prints all tests. CMake
  registers one CTest entry per suite.
- The test plan, the full table of 126 automated tests and the manual checklist are in
  [QA.md](QA.md).
