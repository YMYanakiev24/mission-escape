# Mission Escape: Coding Standards

These rules apply to every file in `src/` and `tests/`. Code quality counts for half of the
grade, so the build enforces as many of them as it can.

## 1. Language and build

- **C++17**, no compiler extensions (`CMAKE_CXX_EXTENSIONS OFF`).
- **Zero warnings.** Project targets compile with `/W4 /permissive- /WX` on MSVC and with
  `-Wall -Wextra -Wpedantic -Wshadow -Werror` on GCC/Clang.
  - Warnings-as-errors is on by default (`MISSION_ESCAPE_WARNINGS_AS_ERRORS`).
  - The flags apply only to our targets, never to raylib; raylib's headers are included as
    system headers.
- **Formatting:** `clang-format` with the committed `.clang-format` (LLVM base, 4-space
  indent, 100 columns, `int* pointer` alignment). Format before every commit.
- Every header starts with `#pragma once`.
- The working language is **English**: identifiers, comments, docs and in-game text.

## 2. Naming

| Kind | Style | Examples |
|---|---|---|
| Types, classes, structs, enums | `PascalCase` | `Grid2D`, `LevelSnapshot`, `PerkType` |
| Enum values | `PascalCase` | `Direction::North`, `LevelState::Caught` |
| Functions and methods | `camelCase` | `tryMove()`, `bfsDistances()`, `mazeToText()` |
| Local variables and parameters | `camelCase` | `deltaSeconds`, `mazeSize` |
| Private / protected data members | `m_camelCase` | `m_cells`, `m_elapsed` |
| Public fields of plain data structs | `camelCase` (no prefix) | `LevelSpec::hasFog`, `Cell::x` |
| Constants and `constexpr` values | `UPPER_SNAKE_CASE` | `MAX_MAZE_SIZE`, `FOG_RADIUS` |
| Namespaces | `lowercase` | `core`, `io`, `render`, `ui`, `audio`, `app` |
| Files | named after the main class | `MazeGenerator.h/.cpp`, `Renderer3D.cpp` |

Names are meaningful words. Single letters are allowed only for loop indices and for `x`/`y`
coordinates (and `t` for a normalised 0..1 parameter in easing code). No cryptic
abbreviations.

A `struct` is used for plain data whose fields are public (e.g. `Cell`, `LevelSpec`,
`HudData`, and `AppContext` as a bundle of shared services). A `class` is used when there are
invariants to protect; its data members are private and prefixed with `m_`.

## 3. Comments

- Every public class and every public function in a header has a Doxygen brief (`///`).
  Parameters are referenced with `@p name`; non-obvious behaviour is spelled out (ownership,
  what happens on invalid input, what is thrown).
- Comments explain **why**, not what. A comment such as
  `// stable_sort keeps equal candidates in scan order, so the result is portable` is welcome;
  `// increment i` is not.
- No commented-out code.

## 4. Structure

- Small, single-purpose functions: **about 40 lines at most**. Longer logic is split into
  named helpers (often in an anonymous namespace in the `.cpp`).
- `main.cpp` only creates `app::App` and runs it.
- No god classes. Each module has one job: `core::Level` simulates; `render::Renderer2D` draws;
  `app::GameSession` translates input and events; `io::SaveSystem` reads and writes files.
- **Layering:**
  - `src/core` depends only on the C++ standard library: **no raylib, no `<iostream>`, no
    `cin`/`cout`/`printf`**. It can be unit-tested headless.
  - `src/io` adds files (`<fstream>`, `<filesystem>`) but still no raylib.
  - Renderers read core state; only commands such as `Level::tryMove()` change it.

Verification (must print nothing):

```bash
grep -rnE "iostream|std::cout|std::cin|printf|raylib\.h|rlgl\.h" src/core
```

## 5. Data types

- `enum class` for states, directions, tiers, perks, effects, skins, screens and sounds.
- Maze cells are a `uint8_t` bitmask: bits 0–3 are walls N/E/S/W, bits 4–7 secret doors
  N/E/S/W.
- `int` for grid coordinates and sizes, `float` for seconds, `bool` for flags, `std::uint32_t`
  for seeds.
- Times are stored in save files as integer **milliseconds**.
- Const-correctness everywhere (`const` references, `const` member functions, `noexcept` on
  trivial accessors).
- No mixing of signed and unsigned values: explicit `static_cast<std::size_t>` at container
  boundaries.

## 6. Memory and resources

- Every per-cell grid is a `core::Grid2D<T>`: an RAII 2D array allocated with
  `new[]`/`delete[]` that follows the rule of five. **No** `std::vector`/`std::array` is used
  for per-cell maps. Short lists (stacks, queues, perk lists) may use `std::vector`.
- raylib resources are owned by move-only RAII wrappers (`RenderTarget`, `TextureResource`,
  `ModelResource`, `ShaderResource`, `Theme` for the font, `AudioManager` for sounds). Objects
  that own GPU or audio resources are destroyed before the window and audio device guards
  close the context.
- No raw owning pointers outside `Grid2D`; ownership elsewhere uses `std::unique_ptr` or plain
  members.

## 7. Constants

- **No magic numbers in game logic.** Every gameplay tuning value lives in
  `src/core/Config.h` (sizes, speeds, durations, star factors, perk counts, file limits).
- Rendering and layout constants are named `constexpr` values at the top of the `.cpp` that
  uses them (e.g. `FOG_FADE_RATE`, `TILE_WIDTH`). Small proportions inside a single drawing
  routine (for example "the eye is 0.3 of the radius from the centre") may stay inline, next to
  the code they shape.
- Colours come only from the tokens in `src/render/Palette.h`, optionally faded or mixed.

## 8. Globals and headers

- No mutable global variables. Shared state lives in `app::AppContext`, owned by `App`.
  (Function-local `static` objects are used only for constants and the test registry.)
- Never `using namespace` in a header. Test `.cpp` files may write `using namespace core;`.
- Include what you use; project headers use paths relative to `src/` (`#include "core/Maze.h"`).

## 9. Errors and validation

- Validate all external input: maze size, player name, slot number, settings values and every
  field of a save file.
- Invalid saves are reported as *corrupted*. The game never crashes on malformed input.
- `Grid2D::at` throws `std::out_of_range`; generation and simulation code uses checked
  coordinates.

## 10. Git

- Small, focused commits with an imperative title using a conventional prefix
  (`feat(core): …`, `fix(render): …`, `docs: …`) and a body that explains *what and why*.
- Never commit build output, `saves/`, `settings.cfg` or `screenshots/` (see `.gitignore`).
