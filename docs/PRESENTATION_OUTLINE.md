# Mission Escape: Presentation Outline (about 10 slides, 10–12 minutes)

## Slide 1: Title
- *Mission Escape*: a C++17 / raylib maze game with a 2D and a 3D mode.
- Author: YMYanakiev24. Claude (Anthropic's AI) wrote the code and docs from the author's
  prompts.
- Visual: main menu screenshot (`docs/screenshots/02_main_menu.png`).

## Slide 2: The assignment and how it was met
- Escape an N × N maze (10–50) from top-left to bottom-right.
- Mazes in dynamic memory (`Grid2D` with `new[]`/`delete[]`), a random maze every run, arrow
  keys, a text view, a menu, input validation, no console I/O in the logic.
- One line per requirement with a ✔ and where it lives in the code.

## Slide 3: Live demo (2–3 minutes)
- New game wizard: an invalid size shows the error, then a valid one.
- Level 1 in 2D, show the T text view and a hover tooltip, then finish the level (star reveal).
- Switch to 3D in Level Select and walk a corridor; show the minimap.

## Slide 4: Architecture
- Layer diagram from `TECHNICAL.md`: core (pure C++) → io → render / ui / audio → app.
- Why: the core is unit-testable without a window; renderers only read the core state; input
  becomes commands such as `tryMove()`.

## Slide 5: Dynamic memory: `Grid2D<T>`
- Diagram: a row-pointer table plus one contiguous block.
- Rule of five, exception safety, `at()` bounds checks.
- Used for walls, fog memory, BFS distances, visited flags, decoy trees, the text canvas.

## Slide 6: Algorithms (from `ALGORITHMS.md`)
- Iterative DFS generation (explicit stack). The menu background *is* this algorithm, animated.
- BFS: shortest paths, par time, compass, chaser steps, secret door validation.
- Tier 2: best of 8 candidates plus decoy edge swaps. Tier 4: braiding.
- Show the maze-generation and chaser flowcharts.

## Slide 7: Tiers, perks and stars
- Table of the four tiers and five perks.
- Star formula: `par = L × 0.30 × tier × mode`; 3★ ≤ 2.0 × par, 2★ ≤ 3.5 × par.
- Screenshots: fog of war and the Hunter.

## Slide 8: Presentation layer
- 2D: cached neon walls with layered glow, follow camera, particles, fades.
- 3D: all walls in one mesh, GLSL fog shader with fallback, 60 FPS on 50 × 50.
- Procedural audio: every sound synthesised in code, seamless loops.

## Slide 9: Quality assurance
- 126 automated tests in 16 suites with the project's own harness; CTest; Debug and Release.
- Simulated play-through, save corruption tests, exact snapshot resume.
- Zero warnings with strict flags, clang-format, the `--screenshots` tour, the manual checklist.

## Slide 10: Process, lessons and next steps
- Process: backlog epics and user stories (see `BACKLOG.md`); Claude wrote the code and docs from
  the author's prompts.
- What was hard: deterministic generation across compilers, premultiplied-alpha glow,
  deferred session changes during fades.
- Possible next steps: online leaderboards, a level editor, gamepad support.
- Questions.
