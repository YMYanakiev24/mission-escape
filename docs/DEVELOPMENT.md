# Development

## Requirements

- CMake 3.20 or newer.
- A C++17 compiler. Tested with Visual Studio 2026 (MSVC).
- Internet access on the first configure: CMake downloads raylib 6.2 (a pinned version checked
  with SHA-256).

## Building

```bash
cmake -S . -B build
cmake --build build --config Release
```

This produces five targets:

| Target | Kind | Contents |
|---|---|---|
| `maze_business` | static library | `src/business` – the game logic |
| `maze_data` | static library | `src/data` – save and settings files |
| `maze_presentation` | static library | `src/presentation` – screens, rendering, sound |
| `maze_game` | executable | `src/main.cpp` – wires the three layers together |
| `maze_tests` | executable | the unit tests |

Warnings are errors (`/W4 /WX` with MSVC, `-Wall -Wextra -Wpedantic -Wshadow -Werror` with GCC
and Clang). After a build the `assets` folder is copied next to the game.

## Running

```bash
build/Release/maze_game
build/Release/maze_game --screenshots
```

The second command walks through the screens and levels by itself and writes PNG captures to
`screenshots/`, using a profile that only exists in memory.

## Tests

```bash
ctest --test-dir build -C Release
build/Release/maze_tests --list
build/Release/maze_tests --suite profiles
```

The test harness is self-written (`tests/TestHarness.h`). Every test function, what it checks
and how to run the manual checklist are described in [QA.md](QA.md).

## Code style

- Formatting with `.clang-format` (100 columns).
- Every gameplay number is a named constant in `src/business/core/Config.h`.
- Resources (memory, textures, window, sound) are managed with RAII.
- Public functions have a `///` comment that says what they do.

## Adding things

**A new rule or perk** – in `src/business/core`. For a perk: add it to `PerkType`,
`ALL_PERK_TYPES` and `PERK_TYPE_COUNT`, give it a name, description and tiers, add an
`EffectType` if it is timed and start it in `Level::applyPerk()`; then draw its icon in the
presentation layer and add tests.

**A new save field** – the model (`SaveData`, `LevelSnapshot`) is in the business layer, writing
and reading in `src/data/io/SaveSystem.cpp`. Read the new key only if it exists, so older files
still load.

**A new screen** – add a value to `ScreenId`, a class derived from `ui::Screen`, and create it in
`createScreen()` in `ScreenStack.cpp`.

**A new kind of storage** – write a class that implements `core::SaveRepository` (or
`SettingsRepository`) and create it in `src/main.cpp` instead of `io::SaveSystem`.
