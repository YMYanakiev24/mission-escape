# Mission Escape: Algorithm Flowcharts

These Mermaid diagrams are the algorithmic scheme of the project. GitHub renders them directly;
they can also be pasted into <https://mermaid.live>.

## 1. Screen state machine and main loop

```mermaid
stateDiagram-v2
    [*] --> Splash
    Splash --> MainMenu : any key / 3 s
    MainMenu --> NewGame : New Game
    MainMenu --> LoadGame : Load Game
    MainMenu --> LevelSelect : Continue (no saved level)
    MainMenu --> Gameplay : Continue (saved level)
    MainMenu --> Settings : push
    MainMenu --> Help : push
    MainMenu --> [*] : Quit
    NewGame --> LevelSelect : profile created
    NewGame --> MainMenu : Esc on first step
    LoadGame --> LevelSelect : Load
    LoadGame --> Gameplay : Resume Level X
    LoadGame --> MainMenu : Back
    LevelSelect --> Gameplay : pick unlocked level
    LevelSelect --> MainMenu : Back
    state Gameplay {
        [*] --> TierIntro : first visit of tier
        [*] --> Playing
        TierIntro --> Playing : Enter
        Playing --> Paused : Esc / P / focus lost
        Paused --> Playing : Resume
        Paused --> Playing : Restart Level
    }
    Gameplay --> Settings : push (from pause)
    Gameplay --> Help : push (from pause)
    Settings --> Gameplay : pop
    Help --> Gameplay : pop
    Gameplay --> LevelComplete : exit reached (push overlay)
    Gameplay --> Caught : hunter caught you (push overlay)
    Gameplay --> MainMenu : Save and Exit
    Gameplay --> LevelSelect : Quit Level (0 stars)
    LevelComplete --> Gameplay : Next / Retry
    LevelComplete --> LevelSelect : Level Select
    LevelComplete --> GameComplete : Finish (level 20)
    Caught --> Gameplay : Retry
    Caught --> LevelSelect : Level Select
    GameComplete --> LevelSelect
    GameComplete --> MainMenu
```

```mermaid
flowchart TD
    A([Start]) --> B[Open window, audio device, load font and saves]
    B --> C{Window closed or quit requested?}
    C -- yes --> Z([Release resources in reverse order and exit])
    C -- no --> D[dt = min of frame time and 0.1 s]
    D --> E[Poll input into one InputState]
    E --> F[Update top screen]
    F --> G{Navigation request pending?}
    G -- yes --> H[Fade out; on the black frame apply the session change and swap screens; fade in]
    G -- no --> I[Update audio: volumes, music track, stream]
    H --> I
    I --> J[Draw visible screens, overlays last, then the fade]
    J --> C
```

## 2. Maze generation for a level

```mermaid
flowchart TD
    S([seed, level, size N]) --> A["levelSeed = mix of seed, level, N (SplitMix64)"]
    A --> B{Tier}
    B -- Labyrinth --> C[Generate 8 perfect mazes with DFS]
    C --> D["score = 3 x shortest path + dead ends; keep the best"]
    D --> E[Add up to 3 exit decoys by tree-preserving edge swaps]
    B -- Awakening / Fog / Hunted --> F[Generate one perfect maze with DFS]
    F --> G{Hunted?}
    G -- yes --> H[Braid: open about 40% of dead ends into loops]
    G -- no --> I
    E --> I{Tier 2 or higher?}
    H --> I
    I -- yes --> J[BFS from start and from exit; walls that save at least 4 steps are candidates]
    J --> K[Shuffle candidates and place 1-3 secret doors not sharing a cell]
    I -- no --> L
    K --> L[Spawn perks on reachable cells, not near start, not on exit]
    L --> M([Maze, secret doors, perks])
```

```mermaid
flowchart TD
    A([DfsMazeBuilder: all walls closed]) --> B["mark (0,0) visited; push it"]
    B --> C{stack empty?}
    C -- yes --> Z([perfect maze, N squared minus 1 passages])
    C -- no --> D[current = top of stack]
    D --> E{unvisited neighbours?}
    E -- no --> F[pop: backtrack] --> C
    E -- yes --> G[pick one at random with the portable Rng]
    G --> H[carve wall between current and neighbour on both sides]
    H --> I[mark neighbour visited and push it] --> C
```

## 3. Move and collision resolution

```mermaid
flowchart TD
    A([Key pressed or held]) --> B{3D mode?}
    B -- yes --> C[Translate: forward = facing, back = opposite, strafe = left or right of facing]
    B -- no --> D[Translate: arrow = compass direction]
    C --> E{Level finished?}
    D --> E
    E -- yes --> X([Inactive])
    E -- no --> F{Player still moving?}
    F -- yes --> G{fresh key press?}
    G -- yes --> H[Buffer the move for when this step ends] --> Y([Busy])
    G -- no --> Y
    F -- no --> I{Neighbour outside the maze?}
    I -- yes --> J[Blocked: bump sound] --> W([Blocked])
    I -- no --> K{Wall between cells?}
    K -- no --> M[Start move]
    K -- yes --> L{Phasing, or a secret door?}
    L -- no --> J
    L -- yes --> M
    M --> N[Start timer on first move, emit events, reveal fog at destination]
    N --> O[Tween over 0.12 s in 2D or 0.24 s in 3D]
    O --> P[On arrival: collect perk, refresh compass path]
    P --> Q{Arrived on the exit?}
    Q -- yes --> R([Won: compute stars])
    Q -- no --> S{Buffered move?}
    S -- yes --> I
    S -- no --> T([Idle])
```

## 4. Chaser step (tier 4)

```mermaid
flowchart TD
    A([Each frame]) --> B{Timer started?}
    B -- no --> Z([wait])
    B -- yes --> C{Chaser active?}
    C -- no --> D[grace = grace - dt]
    D --> E{"grace <= 0?"}
    E -- no --> Z
    E -- yes --> F["Spawn on a random cell at least N/2 steps from the player and 4 from the exit"] --> Z
    C -- yes --> G{Frost Shard active?}
    G -- yes --> Z
    G -- no --> H[Advance current step tween]
    H --> I{Step finished?}
    I -- no --> K
    I -- yes --> J["Fresh BFS from the player's cell, secret doors closed; move to the neighbour with distance minus 1"]
    J --> J2["duration = player step / speed; speed = min of 0.95 and base + 0.002 x active seconds"]
    J2 --> K[Distance to player via BFS gives the warning level]
    K --> L{Interpolated gap below 0.55 cells, or same cell?}
    L -- yes --> M([Caught: 0 stars, screen shake])
    L -- no --> Z
```

## 5. Star calculation

```mermaid
flowchart TD
    A([Level won at time t]) --> B["L = door-free shortest path length from BFS"]
    B --> C["par = L x 0.30 s x tier multiplier x mode multiplier"]
    C --> D{"t <= 2.0 x par?"}
    D -- yes --> S3([3 stars])
    D -- no --> E{"t <= 3.5 x par?"}
    E -- yes --> S2([2 stars])
    E -- no --> S1([1 star])
    S3 --> R
    S2 --> R
    S1 --> R[updateRecord: best stars = max, best time = min]
    R --> U[Unlock next level; unlock skin at levels 5, 10, 15, 20]
    U --> V([Autosave])
    Q([Quit or caught]) --> Q0([0 stars: records unchanged])
```

## 6. Fog of war visibility

```mermaid
flowchart TD
    A([Player cell changes or Lantern toggles]) --> B[visible = none]
    B --> C["queue = player cell at depth 0; mark seen"]
    C --> D{queue empty?}
    D -- yes --> Z([visible cells are also added to explored memory])
    D -- no --> E[take cell, depth]
    E --> F{depth >= radius?}
    F -- yes --> D
    F -- no --> G[for each direction with an OPEN passage]
    G --> H{neighbour already seen?}
    H -- yes --> D
    H -- no --> I["mark visible and explored; enqueue at depth + 1"] --> D
```
