# Mission Escape: Product Backlog

Ready to copy into GitHub Projects. Each task carries a tag for its kind of work:
**[Scrum Trainer]**, **[Frontend]**, **[Backend]** or **[QA]**. Status reflects the delivered
project (✅ done).

This is a one-person project: YMYanakiev24 owns every task, so the tags do not stand for separate
people. The [README](../README.md#author) says how the project was built.

Story points use the Fibonacci scale. Acceptance criteria are written so QA can check them
directly (see [QA.md](QA.md)).

---

## Epic 1: Project foundation

**US-1.1** As a developer, I want a reproducible build so that every machine builds the same
binary. *(3 pts)* ✅
- [x] [Backend] CMake project, C++17, raylib 6.2 pinned with FetchContent (URL + SHA-256)
- [x] [Backend] Warning flags (`/W4 /WX`, `-Wall -Wextra -Wpedantic -Wshadow -Werror`) on our targets only
- [x] [QA] CTest integration, one entry per test suite
- [x] [Scrum Trainer] `.clang-format`, `.gitignore`, coding standards document

**US-1.2** As the developer, I want a clear plan so that work can be split into milestones. *(2 pts)* ✅
- [x] [Scrum Trainer] `docs/PLAN.md` with milestones and a decision log
- [x] [Scrum Trainer] This backlog and the presentation outline

## Epic 2: Maze core

**US-2.1** As a player, I want a new random maze every time so that no two runs are the same.
*(5 pts)* ✅
- [x] [Backend] `Grid2D<T>` with `new[]`/`delete[]`, rule of five
- [x] [Backend] `Maze` bitmask model with both-sided wall maintenance
- [x] [Backend] Iterative DFS generator (explicit stack), portable seeded `Rng`
- [x] [QA] Tests: reachability, N² − 1 passages, start/exit, same seed → same maze

**US-2.2** As a player, I want to choose a maze size from 10 to 50 so that I control the
challenge. *(2 pts)* ✅
- [x] [Backend] `validateMazeSize` for text and numbers with clear messages
- [x] [QA] Boundary tests: 9, 10, 50, 51, negative, non-numeric, overflow

**US-2.3** As a player, I want to see the maze as text so that I can read the layout as
characters. *(2 pts)* ✅
- [x] [Backend] Pure `mazeToText()` returning a string
- [x] [Frontend] Text view overlay (T), drawn on a fixed character grid
- [x] [QA] Output dimensions are 2N+1 × 2N+1; walls and markers are correct

## Epic 3: 2D gameplay

**US-3.1** As a player, I want smooth grid movement that respects walls so that the maze feels
solid. *(5 pts)* ✅
- [x] [Backend] `Level::tryMove`, buffered input, BFS utilities
- [x] [Frontend] `Renderer2D` with cached neon walls, follow camera for large mazes
- [x] [QA] Movement and collision tests, simulated play-through

**US-3.2** As a player, I want a timer and stars so that I can measure my improvement.
*(3 pts)* ✅
- [x] [Backend] `Scoring`: par time, star thresholds, records; a quit gives 0 stars
- [x] [Frontend] HUD with timer, next star limit, coordinates, inventory
- [x] [QA] Exact threshold tests, record tests

**US-3.3** As a player, I want to pause at any time so that I can take a break without losing
time. *(2 pts)* ✅
- [x] [Frontend] Pause menu (Esc/P, focus loss) with Restart, Settings, Help, Save & Exit, Quit (confirmed)

**US-3.4** As a player, I want to hover a cell to see what it is. *(1 pt)* ✅
- [x] [Frontend] Tooltip with coordinates and type (path, exit, secret door, perk)

## Epic 4: Menus and flow

**US-4.1** As a player, I want menus that work with the mouse and the keyboard. *(5 pts)* ✅
- [x] [Frontend] Widget library (Button, Slider, Toggle, TextField) and spatial focus navigation
- [x] [Frontend] Screen stack with fades: Splash, Main Menu, New Game, Load, Level Select, Settings, Help, results
- [x] [QA] Manual checklist: keyboard-only and mouse-only runs

**US-4.2** As a player, I want a guided new-game wizard so that I cannot enter invalid data.
*(3 pts)* ✅
- [x] [Frontend] Slot (overwrite confirmation), name (1–16 alphanumeric), size slider + field, view mode

## Epic 5: Persistence

**US-5.1** As a player, I want three save slots so that several people can play on one
computer. *(5 pts)* ✅
- [x] [Backend] key=value save format with version header, atomic writes, strict validation
- [x] [Frontend] Load Game screen with details, Load, Delete (confirmed), corrupted state
- [x] [QA] Round trip, garbage, truncated, invalid fields, duplicate keys, corrupted file on disk

**US-5.2** As a player, I want to stop mid-level and continue later exactly where I was.
*(3 pts)* ✅
- [x] [Backend] `LevelSnapshot` (position, time, inventory, effects, perks, fog, chaser); regenerate maze from the seed
- [x] [QA] Exact snapshot resume test

**US-5.3** As a player, I want my settings remembered. *(1 pt)* ✅
- [x] [Backend] `settings.cfg` with clamping and defaults
- [x] [QA] Clamping and broken-file tests

## Epic 6: Difficulty tiers

**US-6.1** As a player, I want later levels to be harder in new ways. *(8 pts)* ✅
- [x] [Backend] Tier 2: best of 8 candidates, exit decoys (tree-preserving edge swaps)
- [x] [Backend] Tier 3: fog of war (flood fill through open passages)
- [x] [Backend] Tier 4: braiding and the chaser (BFS step each move, grace period, speed ramp)
- [x] [Backend] Secret doors, verified with BFS to shorten the path
- [x] [Frontend] Fog overlay with exit beacon, chaser art, danger vignette, Caught screen, tier intro cards
- [x] [QA] Tests: decoys keep the maze perfect, braiding, door savings, fog never sees through walls, chaser steps and catching

## Epic 7: Perks and skins

**US-7.1** As a player, I want perks that change how I play. *(5 pts)* ✅
- [x] [Backend] Spawn rules, inventory (3 berries), phase, hourglass, compass, lantern, frost
- [x] [Frontend] Perk icons, particles, toasts, effect bars, compass trail, translucent walls while phasing
- [x] [QA] Spawn rules, inventory cap, phase vs boundary, timer freeze, compass path, lantern, frost

**US-7.2** As a player, I want to unlock skins as rewards. *(3 pts)* ✅
- [x] [Backend] Unlock rules (levels 5/10/15/20, first completion with ≥1 star)
- [x] [Frontend] Five animated skins, Settings → Skins with previews, unlock announcement
- [x] [QA] Unlock rule tests

## Epic 8: Audio

**US-8.1** As a player, I want sound and music without downloaded assets. *(5 pts)* ✅
- [x] [Frontend] Synthesiser: 11 effects and 5 seamless ambient loops
- [x] [Frontend] Audio manager with volume settings, crossfades, optional `.ogg` overrides
- [x] [QA] Tests: no clipping, silent edges, seamless loops, determinism

## Epic 9: 3D first-person mode

**US-9.1** As a player, I want to walk the maze in first person. *(8 pts)* ✅
- [x] [Frontend] Single-mesh walls, textured floor and ceiling, distance-fog shader with fallback
- [x] [Frontend] Portal, perks, doors and chaser in 3D; hand light; camera shake
- [x] [Frontend] Minimap (M toggles size) showing explored cells, the arrow, seen items and the chaser
- [x] [Backend] Facing and turning in the core; relative movement mapping
- [x] [QA] 50 × 50 performance check at 60 FPS (screenshot tour)

## Epic 10: Quality and delivery

**US-10.1** As the grader, I want clean, documented code. *(3 pts)* ✅
- [x] [Scrum Trainer] Coding standards; review that functions stay near 40 lines
- [x] [Backend] Doxygen briefs on all public API
- [x] [QA] Zero-warning builds in Debug and Release; no I/O in `src/core` (grep)

**US-10.2** As the developer, I want to present the project confidently. *(3 pts)* ✅
- [x] [Scrum Trainer] README, game rules, technical documentation, algorithm flowcharts
- [x] [QA] QA document with test table and manual checklist
- [x] [Frontend] `--screenshots` tour and screenshots in the README
- [x] [Scrum Trainer] Presentation outline

---

## Sprint suggestion (if the work is re-planned)

| Sprint | Goal | Epics |
|---|---|---|
| 1 | Playable 2D maze | 1, 2, 3 |
| 2 | Complete game loop | 4, 5 |
| 3 | Depth | 6, 7 |
| 4 | Atmosphere | 8, 9 |
| 5 | Hardening and defence | 10 |
