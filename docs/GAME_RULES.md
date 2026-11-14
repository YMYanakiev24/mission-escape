# Game description and rules

## Goal

Mission Escape is a maze game. You start in the top-left cell (0, 0) of an N × N maze and have
to reach the glowing exit portal in the bottom-right cell. The faster you get there, the more
stars you earn (up to 3). You choose N, from 10 to 50, when you create a game, and every attempt
generates a new maze.

## Starting a game

1. **Main Menu** – Continue (resumes the last profile), New Game, Load Game, Settings, Help, Quit.
2. **New Game** – a four-step wizard: pick a save slot (1–3), enter a name (1–16 letters, digits
   and spaces), choose the maze size (10–50, with the slider or by typing a number) and choose
   the view (2D Classic or 3D First-Person). Each step validates its input before you can go on.
3. **Level Select** – the 20 levels grouped by tier. A level unlocks when you finish the one
   before it. You can also switch the view here.
4. **Load Game** – the three slots with name, last played date, progress, size and view. Load,
   resume or delete a profile here. A damaged save file is shown as "Save corrupted" and can be
   deleted; it never crashes the game.

## The maze

- Walls are drawn as glowing lines (2D), walls in a corridor (3D) or `#` characters (text view).
- You move one cell at a time. You cannot move through walls or out of the maze.
- **Secret doors** are walls marked with a green rune. You can walk through them, and each one
  is guaranteed to shorten the way to the exit by at least 4 steps.
- The exit is a glowing green portal in the bottom-right cell.

## Tiers

| Tier | Levels | Rules |
|---|---|---|
| Awakening | 1–5 | A perfect maze: exactly one route leads to the exit. No secret doors. |
| Labyrinth | 6–10 | Longer routes, wrong branches right next to the exit, 1–3 secret doors and stricter star times. |
| Fog | 11–15 | You only see 3 cells around you, and never through walls. Explored cells stay remembered; the exit glows faintly through the dark. |
| Hunted | 16–20 | The maze has loops, and a Hunter appears 3 seconds after your first move. It follows the shortest path to you but cannot use secret doors. If it catches you, the attempt ends with 0 stars. |

The first time you enter a tier, a card explains its rules.

## Perks

| Perk | Effect | Tiers |
|---|---|---|
| Magic Berry | Carried, up to 3. Press Shift to pass through inner walls for 3 s (never the outer wall). | All |
| Hourglass | Freezes the level timer for 5 s. | All |
| Compass Spark | Lights the shortest path to the exit for 3 s. | 2 and up |
| Lantern | Doubles your sight radius in the fog for 10 s. | 3 |
| Frost Shard | Freezes the Hunter for 4 s. | 4 |

A Magic Berry picked up with a full inventory stays on the floor for later.

## Controls

| Action | 2D | 3D | Gamepad |
|---|---|---|---|
| Move | Arrow keys / WASD (hold to keep walking) | W / S forward and back | D-pad or left stick |
| Turn 90° | – | A / D | D-pad or stick left/right |
| Strafe | – | Q / E | LB / RB |
| Use a Magic Berry | Shift | Shift | X or RT |
| Text view (ASCII) | T | T | Y |
| Larger/smaller minimap | – | M | LT |
| Restart the level | R | R | Select |
| Pause | Esc / P | Esc / P | Start |
| What is this cell? | Hover the mouse over it | – | – |

In menus, the arrow keys (or D-pad) move the highlight, Enter (A) confirms and Esc (B) goes back.
Everything also works with the mouse.

## Text view

Press T to see the maze as text from above. `#` is a wall, a space is an opening, `%` a secret
door, `S` the start, `E` the exit, `@` you, `X` the Hunter, `*` a perk and `.` an unexplored cell
in the fog.

## What the screen shows

- **HUD (top)**: level number, tier, view, your personal best (green while you can still beat
  it), the timer, the best star rating still reachable and how long it stays reachable, your
  current cell and your Magic Berries.
- **Effects**: active perks with countdown bars.
- **Bottom strip**: a reminder of the controls.
- **3D**: a minimap of the explored maze in the top-right corner.
- **Tooltips (2D)**: hover a cell to see what it is (path, start, exit, secret door, perk,
  the Hunter or unexplored).

## Stars and records

- The timer starts with your first move, stops while an Hourglass is active and stops while the
  game is paused.
- **Par time** = shortest path length × 0.30 s × tier factor (Awakening 1.0, Labyrinth 0.85,
  Fog 1.5, Hunted 1.0) × view factor (2D 1.0, 3D 1.75).
- **3 stars** up to 2 × par, **2 stars** up to 3.5 × par, otherwise **1 star**.
- Quitting a level or being caught gives 0 stars and never changes your records.
- The results screen shows your time, stars, best result and how many steps you took compared
  with the shortest route.

## Pausing and saving

Esc or P opens the pause menu, and the timer stops: Resume, Restart, Settings, Help, Save & Exit
and Quit Level. **Save & Exit** stores the level exactly where you are; Continue on the main menu
resumes it. Quit Level gives 0 stars but keeps your previous best. The game saves automatically
after every finished level.

## Skins and settings

- **Skins** change how your player looks. Wisp is available from the start; Prism, Comet,
  Phantom and Nova unlock when you finish levels 5, 10, 15 and 20.
- **Audio**: master, music and effects volume, mute.
- **Display**: fullscreen, FPS counter, head bob in 3D, screen shake when caught.
