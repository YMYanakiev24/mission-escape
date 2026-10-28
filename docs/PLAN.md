# Mission Escape: Implementation Plan

## Goal

A polished C++17 / raylib maze game: a 2D top-down mode with neon visuals and a special 3D
first-person mode. It is built for a graded school assignment, so the hard requirements
(dynamic `Grid2D`, maze size 10–50, start (0,0) and exit (N-1,N-1), text view, a menu, input
validation, no standard I/O in the logic) take priority over everything else.

## Architecture in one picture

```
main.cpp ─► app::App (window, screen state machine, main loop)
              │
              ├─ ui::*Screen  (one class per screen, Widgets + FocusGroup for mouse/keyboard)
              ├─ app::GameSession (gameplay controller: input → core commands, events → fx/audio)
              ├─ render::* (Renderer2D, Renderer3D, Minimap, Hud, SkinRenderer, Effects)
              ├─ audio::*  (Synth: procedural samples, AudioManager: sfx + streamed music)
              └─ io::*     (SaveSystem slots, SettingsStore) ─┐
                                                              ▼
                         core::* (pure C++17, no raylib, no iostream; unit-tested headless)
                         Grid2D · Maze · MazeGenerator · Pathfinding · Level · Player · Chaser
                         Perks · Fog · Scoring · LevelRules · Progress · Validation · TextView
```

* `maze_core`: a static library with `src/core` and `src/io`. It is pure C++ and links nothing
  graphical.
* `maze_game`: the executable. It contains everything that touches raylib.
* `maze_tests`: a self-written test harness, registered with CTest one suite at a time.

## Milestones

| # | Milestone | Content |
|---|-----------|---------|
| 1 | Scaffold | CMake, raylib fetch pinned to 6.2, warnings, window, palette, state machine stub |
| 2 | Core | `Grid2D`, `Maze`, iterative DFS, BFS, `mazeToText`, validation, test harness |
| 3 | 2D gameplay | Tweened movement, collision, exit, timer, stars, HUD, tooltips, pause |
| 4 | Menus | Widgets, main menu, new game wizard, level select, help, intro |
| 5 | Persistence | Save slots with mid-level snapshots, `settings.cfg` |
| 6 | Tiers | Candidate selection + decoys, fog, braided maze + chaser, secret doors |
| 7 | Perks | Spawning, inventory, effects |
| 8 | Skins | Five procedural skins and unlock rules |
| 9 | Audio | Synthesised SFX and ambient pads |
| 10 | 3D mode | Single-mesh walls, fog shader, minimap |
| 11 | Polish | Transitions, particles, menu background, tier cards, balance |
| 12 | Docs & QA | All documents, warning sweep, screenshot tour |

## Decisions

Decisions taken while building, with the reason for each.

1. **raylib 6.2** is pinned by URL and SHA-256. It was the latest stable tag when the project
   was set up (released 2026-10-15). `master` is never used.
2. **Repository root = the `maze/` working folder.** The spec's `neon-labyrinth/` tree is
   reproduced inside it.
3. **`io/` is compiled into `maze_core`.** It is pure C++ (only `<fstream>` / `<filesystem>`,
   no raylib), so save files can be tested headless. The spec asks for three targets, and this
   keeps it at three.
4. **Portable randomness.** `std::uniform_int_distribution` and `std::shuffle` are
   implementation-defined, so `core::Rng` does its own unbiased bounded sampling and
   Fisher–Yates shuffle on top of `std::mt19937`. Only the engine output is fixed by the
   standard. This keeps a seed producing the same maze on MSVC, GCC and Clang, which makes save
   files portable.
5. **Seeds.** Each attempt at a level draws a fresh 32-bit seed from `std::random_device`, so
   every run gets a new maze. The level seed is mixed with the level number and maze size
   (SplitMix-style) before generation. Restart and Retry reuse the current seed so the player
   can improve on the same layout. Starting the level again from Level Select rolls a new one.
6. **Times are stored as integer milliseconds** in save files. This avoids locale-dependent
   float formatting and any `printf`-family call in `core/`.
7. **Grid2D layout.** There is one contiguous `new T[w*h]` block plus a `new T*[h]` table of
   row pointers. That gives `grid[y][x]` syntax, two allocations instead of `h+1`, and simple
   exception safety.
8. **Wall bitmask.** Bit set = wall. A secret door is a wall bit *plus* its secret bit, so
   everything that ignores secret doors (the chaser, fog visibility, the plain BFS) treats it as
   a wall automatically.
9. **Tier 2 decoys** use tree-preserving edge swaps. Each swap cuts an off-path subtree from its
   parent and re-attaches it to a cell on the last stretch of the solution path. The maze stays
   perfect (N²−1 passages) and the shortest path is unchanged, but dead-end branches now leave
   from right next to the exit.
10. **Par time ignores secret doors.** It uses the door-free shortest path, so finding a door
    is a real reward.
11. **Fullscreen = borderless windowed** (`ToggleBorderlessWindowed`). It switches instantly
    and never changes the monitor's video mode.
12. **No Unicode glyphs in text.** Stars and icons are drawn procedurally, so they render the
    same with the bundled font and with raylib's fallback font.
13. **Screenshots use `LoadImageFromScreen` + `ExportImage`** into `./screenshots/`. The tour
    runs on an in-memory profile and never touches the real save slots.
14. **Milestones 3 and 4 share one commit.** The gameplay screen, the pause menu and the menu
    state machine were built together, so splitting them would have produced a commit that
    does not build.
15. **The screen stack fades to black before switching.** Session changes (start, retry,
    quit) are deferred to that black frame, so result overlays never show the next level
    behind them.
16. **Screenshots flush the render batch first.** raylib batches geometry until
    `EndDrawing()`, so the capture calls `rlDrawRenderBatchActive()` before
    `LoadImageFromScreen()`.
17. **Exit decoys move whole branches.** A decoy cuts a side branch where it joins the solution
    path and re-attaches it next to the exit. If the last 12 path cells offer no suitable
    branch, the search window doubles, so every maze gets its decoys.
18. **Secret doors are checked one at a time against the door-free maze.** Each door must save
    at least 4 steps on its own, which is exactly what the test verifies.
19. **The chaser is 10% slower in 3D.** First-person play costs turning time and sight, so
    `CHASER_SPEED_3D_FACTOR = 0.9`.
20. **3D walls are one mesh per level.** Walls are merged into straight runs, each run becomes a
    box, and every face is split into floor strip, body and glow band. This is built once,
    drawn in a single call, and holds 60 FPS on 50 × 50.
21. **Fog of war is visibility by passage distance.** It is a flood fill of up to 3 steps
    through open passages, so walls and secret doors always block sight.
22. **Audio is synthesised at start-up**, and music loops lazily on first use. Every oscillator
    frequency is rounded to whole cycles per 16 s loop, which makes the loops seamless.
23. **Windows Smart App Control** on the development machine sometimes blocks freshly built,
    unsigned executables. The code is unaffected. Builds were verified whenever a binary was
    allowed to run, and the security setting was left untouched.
24. **The game was renamed to Mission Escape** after the milestones were finished. The spec called
    it Neon Labyrinth. The tier named Labyrinth and the neon visual style keep their names.

## Status

All 12 milestones are complete. See the README for the feature list and build steps,
[QA.md](QA.md) for the test results and [TECHNICAL.md](TECHNICAL.md) for the design.
