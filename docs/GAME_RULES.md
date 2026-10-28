# Mission Escape: Game Rules

## Goal

Every level is a square maze. You start in the **top-left cell (0, 0)** and must reach the
glowing green **exit portal in the bottom-right cell (N-1, N-1)**. Reaching the exit completes
the level and earns 1 to 3 stars, depending on how fast you were.

## Starting a game

1. Choose one of **three save slots**. Taking an occupied slot asks for confirmation first.
2. Enter a **name**: 1–16 characters, letters, digits and spaces only.
3. Choose the **maze size N**, from **10 to 50**. Every level of this profile uses an N × N
   maze, so difficulty comes from the tier rules, not the size.
4. Choose a **view**: **2D Classic** (top-down) or **3D First-Person**. You can switch later
   in Level Select.

Each attempt at a level generates a new random maze. *Restart* and *Retry* replay the same
maze; choosing the level again in Level Select creates a new one.

## Moving

- You move one cell at a time. Holding a key keeps walking.
- **Walls block you**, with two exceptions:
  - **Secret doors**: wall segments marked with a green rune. You can walk straight through
    them. A secret door is only placed where it shortens the way to the exit.
  - **Magic Berry phase**: for 3 seconds every inner wall is passable. The outer border of the
    maze never is, and every move still ends on a normal cell.
- In 3D, Up/W walks forward, Down/S walks back, Left/Right (or A/D) turn 90° and Q/E strafe.

## Tiers

| Tier | Levels | Name | What changes |
|---|---|---|---|
| 1 | 1–5 | **Awakening** | Classic perfect mazes: exactly one route between any two cells. |
| 2 | 6–10 | **Labyrinth** | The hardest of 8 generated mazes (longest route, most dead ends), plus decoy branches next to the exit. Star times are 15% stricter. Secret doors appear (1–3 per level, also in tiers 3 and 4). |
| 3 | 11–15 | **Fog** | You only see cells within 3 steps along open passages; you never see through walls. Explored cells stay dimly visible. Until you have seen the exit, a faint beacon shows where it is. |
| 4 | 16–20 | **Hunted** | About 40% of dead ends are opened into loops, so you can run circles, but **the Hunter** chases you. |

The first time you enter a tier, an intro card explains what has changed.

### The Hunter (tier 4)

- It appears **3 seconds after your first move**, at least N/2 steps away from you and never
  next to the exit.
- It moves cell by cell along the **shortest path to you, recomputed for every step**.
- Its speed is **70% of yours on level 16, rising to 85% on level 20**, plus a slow increase
  over time (capped at 95%). In 3D mode it moves at 90% of that speed.
- It **cannot use secret doors** and cannot phase through walls.
- If it reaches your cell (or you run into each other mid-step), you are **caught**: the attempt
  ends with 0 stars.
- As it gets closer, the screen edges pulse pink and a heartbeat speeds up.

## Perks

Perks lie on random cells, never on the start cell, its neighbours or the exit. Their number
grows with the maze size (about N/8) and with the tier. Walk onto a perk to collect it.

| Perk | Tiers | Effect |
|---|---|---|
| **Magic Berry** | all | Stored in your inventory (max 3). Press **Shift** to **phase** through inner walls for 3 s. If you already carry 3, the berry stays on the floor. |
| **Hourglass** | all | Freezes the level timer for 5 s. |
| **Compass Spark** | 2–4 | Shows a glowing trail along the shortest path to the exit for 3 s. |
| **Lantern** | 3 | Doubles your sight radius in the fog for 10 s. |
| **Frost Shard** | 4 | Freezes the Hunter for 4 s. |

Active effects are shown with countdown bars under the HUD.

## Timer and stars

- The timer **starts with your first move**. It stops while the game is **paused** and while an
  **Hourglass** is active.
- Every level has a **par time**:

  ```
  parTime = shortestPathLength × 0.30 s × tierMultiplier × modeMultiplier
  ```

  `shortestPathLength` is the length of the shortest route from start to exit **without**
  secret doors, so finding a door is a bonus.

  | Multiplier | Value |
  |---|---|
  | Awakening | × 1.0 |
  | Labyrinth | × 0.85 |
  | Fog | × 1.5 |
  | Hunted | × 1.0 |
  | 2D Classic | × 1.0 |
  | 3D First-Person | × 1.75 |

- **3 stars** if your time ≤ 2.0 × par, **2 stars** if ≤ 3.5 × par, **1 star** for any
  completion. A time exactly on a limit still earns the better rating.
- The HUD always shows the best rating you can still get and its time limit
  (e.g. ★★★ *under 0:42.0*).
- **Quitting** a level (or being caught) gives **0 stars** and never overwrites a previous best.
- Your **best stars** and **best time** are saved for every level.
- Completing a level with at least one star unlocks the next level.

## Skins

| Skin | Unlocked by |
|---|---|
| Wisp | available from the start |
| Prism | completing level 5 |
| Comet (leaves a trail) | completing level 10 |
| Phantom | completing level 15 |
| Nova | completing level 20 |

New skins are announced on the Level Complete screen. Choose one in **Settings → Skins**. In
3D, your skin colours the minimap arrow and the light you carry at the bottom of the screen.

## Saving

- The game **autosaves** after every completed level.
- **Pause → Save & Exit to Menu** stores the level you are playing, including position, time,
  inventory, effects, collected perks, explored fog and the Hunter. **Continue** or
  **Load Game → Resume Level X** puts you back exactly where you were.
- Starting a different level replaces the stored in-progress level.
- A damaged save file is reported as **"Save corrupted"** in Load Game and can be deleted; it
  never crashes the game.

## Other screens and keys

- **T** toggles the **Text View**: the maze drawn as characters. `#` is a wall, `%` a secret
  door, `@` you, `S` the start, `E` the exit, `*` a perk, `X` the Hunter and `.` unexplored fog.
- **M** (3D) toggles between the small and the large minimap.
- **Esc / P** pauses (the game also pauses when its window loses focus). The pause menu offers
  Resume, Restart Level, Settings, Help, Save & Exit to Menu and Quit Level.
- **Settings**:
  - Audio: master, music and effects volume; mute.
  - Skins.
  - Display: fullscreen (borderless), FPS counter, head bob in 3D.
