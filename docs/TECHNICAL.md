# Technical documentation

This document describes how Mission Escape is implemented, area by area: the backend (business
logic and data access), the frontend (presentation) and quality assurance. The game itself is
described in [GAME_RULES.md](GAME_RULES.md); the layer structure in
[ARCHITECTURE.md](ARCHITECTURE.md); the algorithms in [ALGORITHMS.md](ALGORITHMS.md).

## 1. Overview

| Item | Value |
|---|---|
| Language | C++17 |
| Library | raylib 6.2 (window, input, 2D/3D drawing, audio), fetched by CMake |
| Build | CMake 3.20+; warnings are errors (`/W4 /WX`, `-Wall -Wextra -Wpedantic -Wshadow -Werror`) |
| Architecture | Three layers: presentation, business logic, data access |
| Source size | about 17,000 lines in `src/` and `tests/` |
| Tests | 138 test functions in 17 suites (see [QA.md](QA.md)) |

## 2. Backend – business logic layer (`src/business/core`)

Pure C++: no raylib, no files and no standard input/output, so every rule can be unit tested.

### Data structures and dynamic memory

- **`Grid2D<T>`** – the dynamic 2D array behind every per-cell map (walls, fog, distances,
  visited flags). It makes two allocations with `new[]`: one contiguous block of width × height
  elements and a table of row pointers into it, which gives `grid[y][x]` access. It follows the
  rule of five: deep copy, copy-and-swap assignment, moves that steal the buffers, and a
  destructor that frees both arrays with `delete[]`. `at(x, y)` is bounds-checked.
- **`Maze`** – one `std::uint8_t` per cell in a `Grid2D`: bits 0–3 are the walls N/E/S/W and
  bits 4–7 mark the same walls as secret doors. `carvePassage` and `addWall` always update both
  sides of a wall. `canPass(cell, direction, rules)` decides every move; `PassRules` say whether
  a mover may use secret doors (the player) or phase through walls (a Magic Berry).
- **`Cell`, `Direction`** – coordinates and the four compass directions in a fixed N, E, S, W
  order, which keeps every algorithm deterministic.
- **`Config.h`** – every gameplay number (sizes, speeds, durations, multipliers) as a named
  `inline constexpr` constant.

### Maze generation (`MazeGenerator`)

- `DfsMazeBuilder` – the recursive backtracker (randomised depth-first search) with an explicit
  stack, so a 50 × 50 maze cannot overflow the call stack. It can run one step at a time, which
  the animated menu background uses.
- `generateHardestCandidate`, `addExitDecoys`, `braidMaze`, `placeSecretDoors` – the tier rules.
- `generateLevelMaze(spec, size, rng)` – builds a level's maze from its tier rules.
- `core::Rng` – deterministic random numbers on `std::mt19937`, with hand-written `nextInt`
  (rejection sampling) and shuffle, so a seed gives the same maze with every compiler.

### Pathfinding (`Pathfinding`)

Breadth-first search: `bfsDistances`, `shortestPath`, `nextStepToward`, `allCellsReachable`.
Used for the par time, tier generation, secret doors, perk placement, the Hunter and the Compass
Spark trail.

### Level simulation (`Level`, `Player`, `Fog`, `Chaser`, `Perks`)

- `Level` owns one attempt: maze, player, fog, Hunter, perks, effect timers and timer. The
  presentation sends commands (`tryMove`, `bufferMove`, `activateBerry`, `turnLeft`,
  `turnRight`) and calls `update(deltaSeconds)` every frame; the level reports what happened as
  events (`Moved`, `PerkCollected`, `Caught`, `Won`, …).
- `Mover`/`Player` – grid-locked movement with smooth interpolation between cells.
- `Fog` – visible and explored cells; a depth-limited BFS through open passages.
- `Chaser` – the Hunter: a fresh BFS step towards the player whenever it stands still.
- `Perks`/`EffectTimers` – perk placement and the timed effects.
- `snapshot()`/`restore()` – Save & Exit; `restore` validates everything before changing state.

### Scoring, progress and profiles

- `Scoring` – par time, star rating, records, time formatting.
- `Progress` – unlocked levels, records, skins and seen tier intros of one save slot.
- `Validation` – maze size, player name and slot number checks with user-facing messages.
- `ProfileService` – the three save slots, the active profile and the settings: activating,
  creating and deleting profiles, recording a finished level, Save & Exit, autosave. It stores
  data only through the repository interfaces in `Repositories.h`.

## 3. Backend – data access layer (`src/data/io`)

- `KeyValueDocument` – the `key=value` text format: strict parsing (a malformed line or a
  duplicate key invalidates the file), safe reading with a size limit, and atomic writing (a
  temporary file renamed over the real one).
- `SaveSystem` – `saves/slot1..3.sav`; implements `core::SaveRepository`. `parseSave` validates
  every field and reports a damaged file as `Corrupt` instead of crashing.
- `SettingsStore` – `settings.cfg`; implements `core::SettingsRepository`. Unknown or invalid
  values fall back to the defaults.

## 4. Frontend – presentation layer (`src/presentation`)

The only layer that uses raylib.

- **Application (`app/`)** – `App` opens the window (1280 × 720, resizable, 60 FPS) and runs the
  main loop; `AppContext` holds the services all screens share (`profiles`, theme, audio,
  navigator, the running `GameSession`); `ScreenStack` is the screen state machine with fade
  transitions; `Input` maps keyboard, mouse and gamepad to `GameAction`s; `GameSession` connects
  one `Level` to input and rendering; `FeedbackPresenter` turns level events into sound,
  particles and messages; `ScreenshotTour` is the `--screenshots` mode.
- **Screens (`ui/`)** – Splash, Main Menu, New Game, Load Game, Level Select, Settings, Help,
  Gameplay (with the pause menu), Level Complete, Caught and Game Complete. Widgets: `Button`,
  `Slider`, `Toggle`, `TextField` and `FocusGroup` for keyboard and mouse focus.
- **Rendering (`render/`)** – `Renderer2D` (walls cached in an off-screen texture, glow, floor
  lighting, camera that follows the player in large mazes), `Renderer3D` (one mesh for all walls,
  a GLSL distance-fog shader, smooth turns), `Hud`, `Minimap`, `TextViewRenderer` (the ASCII
  view), `Effects` (particles and messages), `Theme` (font and UI scaling) and RAII wrappers for
  GPU resources in `GpuResources.h`.
- **Audio (`audio/`)** – `Synth` creates every sound effect and music loop in code; `AudioManager`
  plays them and crossfades the music between the menu and the tiers.

## 5. Composition root (`src/main.cpp`)

The only file that sees all three layers. It creates `io::SaveSystem` and `io::SettingsStore`
and passes them to `app::App`, which hands them to `core::ProfileService` as
`core::SaveRepository` and `core::SettingsRepository`.

## 6. Quality assurance

The automated tests cover the business and data layers and the sound synthesiser; the screens
are checked with the manual checklist and the `--screenshots` tour. Every test function, the
checklist and the mapping from the assignment's requirements to code and tests are in
[QA.md](QA.md).

## 7. Coding conventions

- Formatting with `.clang-format` (100 columns).
- `PascalCase` for types, `camelCase` for functions and variables, `m_` prefix for members,
  `UPPER_CASE` for constants.
- Every public function has a `///` comment that says what it does.
- Resources (memory, window, audio device, GPU resources, files) are owned through RAII.
- Expected failures are returned as values (`bool`, `std::optional`, `LoadStatus`); exceptions
  are used only for programming errors.
