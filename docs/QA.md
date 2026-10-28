# Mission Escape: Quality Assurance

## 1. Test plan

| Area | Approach | Where |
|---|---|---|
| Game rules (generation, movement, perks, fog, chaser, scoring, unlocks) | Automated unit tests on the pure core | `tests/test_*.cpp` |
| Persistence (saves, settings, snapshots, corrupted files) | Automated tests, in memory and on disk (temporary folder) | `tests/test_saves.cpp` |
| Input validation (size, name, slot) | Automated boundary tests | `tests/test_validation.cpp` |
| End-to-end gameplay | Simulated play-through: a BFS solver plays complete levels | `tests/test_level.cpp` |
| Audio synthesis | Automated tests on the generated samples (no device needed) | `tests/test_audio.cpp` |
| Screens, rendering, layout, contrast | Scripted screenshot tour + manual checklist (section 4) | `maze_game --screenshots` |
| Performance | 50 × 50 maze in 2D and 3D with the FPS counter (tour steps 34 and 35) | screenshot tour |
| Code quality | Zero warnings with `/W4 /WX` (MSVC) or `-Wall -Wextra -Wpedantic -Wshadow -Werror`; clang-format; grep for forbidden I/O in `src/core` | build + `docs/CODING_STANDARDS.md` |

**Entry criteria:** the project configures and builds without warnings in Debug and Release.
**Exit criteria:** all automated tests pass in both configurations, the screenshot tour finishes,
and every manual checklist item has been checked.

### Test harness

The project uses its own minimal harness (`tests/TestHarness.h`), so there are no external
dependencies:

- `TEST_CASE(suite, name)` defines a test function and registers it before `main()` runs.
- `CHECK(condition)` records a failure and carries on. `CHECK_EQ(actual, expected)` also prints
  both values. `REQUIRE(condition)` stops the current test.
- `maze_tests` runs every test; `maze_tests --suite fog` runs one suite; `maze_tests --list`
  prints all names.
- CMake registers **one CTest entry per suite** (16 suites), so `ctest` reports each area
  separately.

## 2. How to run the tests

```bash
cmake -S . -B build
```
```bash
cmake --build build --config Release
```
```bash
ctest --test-dir build -C Release --output-on-failure
```

Run a single suite with more detail:

```bash
build/Release/maze_tests --suite saves
```

Run the screenshot tour. It writes the PNGs to `./screenshots/` and never touches your saves:

```bash
build/Release/maze_game --screenshots
```

## 3. Automated test cases

Last full run: **2026-10-19**, MSVC 14.51 (Visual Studio 2026), x64. All **126 tests passed**
in both **Debug and Release** builds.

| ID | Suite | Test | Expected result | Status |
|---|---|---|---|---|
| T-AUDIO-01 | audio | `everySoundEffectIsAudibleAndNeverClips` | Every sound effect is audible and never clips | Pass |
| T-AUDIO-02 | audio | `soundEffectsStartAndEndSilentlyToAvoidClicks` | Sound effects start and end silently to avoid clicks | Pass |
| T-AUDIO-03 | audio | `musicLoopsHaveExactLengthAndJoinSeamlessly` | Music loops have exact length and join seamlessly | Pass |
| T-AUDIO-04 | audio | `synthesisIsDeterministic` | Synthesis is deterministic | Pass |
| T-CHASER-01 | chaser | `everyStepReducesDistanceToPlayer` | Every step reduces distance to player | Pass |
| T-CHASER-02 | chaser | `neverUsesSecretDoors` | Never uses secret doors | Pass |
| T-CHASER-03 | chaser | `frozenChaserDoesNotMove` | Frozen chaser does not move | Pass |
| T-CHASER-04 | chaser | `spawnsAtLeastHalfTheMazeAwayAndNotNearTheExit` | Spawns at least half the maze away and not near the exit | Pass |
| T-CHASER-05 | chaser | `speedRampsUpButIsCapped` | Speed ramps up but is capped | Pass |
| T-CHASER-06 | chaser | `appearsAfterGracePeriodAndCatchesAnIdlePlayer` | Appears after grace period and catches an idle player | Pass |
| T-CHASER-07 | chaser | `dangerRisesAsTheChaserApproaches` | Danger rises as the chaser approaches | Pass |
| T-FOG-01 | fog | `visibleCellsAreAlwaysWithinRadiusThroughOpenPassages` | Visible cells are always within radius through open passages | Pass |
| T-FOG-02 | fog | `wallsBlockSightEvenToAdjacentCells` | Walls block sight even to adjacent cells | Pass |
| T-FOG-03 | fog | `secretDoorsBlockSight` | Secret doors block sight | Pass |
| T-FOG-04 | fog | `exploredCellsAreRememberedButNotVisible` | Explored cells are remembered but not visible | Pass |
| T-FOG-05 | fog | `encodedExplorationRoundTrips` | Encoded exploration round trips | Pass |
| T-FOG-06 | fog | `fogLevelsRevealOnlyNearThePlayer` | Fog levels reveal only near the player | Pass |
| T-GENERATION-01 | generation | `perfectMazeReachesEveryCell` | Perfect maze reaches every cell | Pass |
| T-GENERATION-02 | generation | `perfectMazeHasExactlyNSquaredMinusOnePassages` | Perfect maze has exactly N squared minus one passages | Pass |
| T-GENERATION-03 | generation | `startIsTopLeftAndExitIsBottomRight` | Start is top left and exit is bottom right | Pass |
| T-GENERATION-04 | generation | `sameSeedProducesIdenticalMaze` | Same seed produces identical maze | Pass |
| T-GENERATION-05 | generation | `differentSeedsProduceDifferentMazes` | Different seeds produce different mazes | Pass |
| T-GENERATION-06 | generation | `boundaryWallsAreNeverCarved` | Boundary walls are never carved | Pass |
| T-GENERATION-07 | generation | `builderStepsVisitEveryCellBeforeFinishing` | Builder steps visit every cell before finishing | Pass |
| T-GENERATION-08 | generation | `wallsStayConsistentOnBothSides` | Walls stay consistent on both sides | Pass |
| T-GENERATION-09 | generation | `derivedLevelSeedsDifferPerLevelAndSize` | Derived level seeds differ per level and size | Pass |
| T-GENERATION-10 | generation | `rngNextIntStaysWithinInclusiveBounds` | Rng nextInt stays within inclusive bounds | Pass |
| T-TIERS-01 | tiers | `tierBoundariesMatchTheCampaignTable` | Tier boundaries match the campaign table | Pass |
| T-TIERS-02 | tiers | `hardestCandidateScoresAtLeastAsHighAsASingleMaze` | Hardest candidate scores at least as high as a single maze | Pass |
| T-TIERS-03 | tiers | `exitDecoysKeepTheMazePerfectAndThePathUnchanged` | Exit decoys keep the maze perfect and the path unchanged | Pass |
| T-TIERS-04 | tiers | `braidingRemovesDeadEndsAndCreatesLoops` | Braiding removes dead ends and creates loops | Pass |
| T-TIERS-05 | tiers | `everySecretDoorShortensThePathToTheExit` | Every secret door shortens the path to the exit | Pass |
| T-TIERS-06 | tiers | `awakeningLevelsHaveNoSecretDoors` | Awakening levels have no secret doors | Pass |
| T-TIERS-07 | tiers | `labyrinthLevelsStayPerfectApartFromDoors` | Labyrinth levels stay perfect apart from doors | Pass |
| T-GENERATION-11 | generation | `levelMazesAreDeterministicPerTier` | Level mazes are deterministic per tier | Pass |
| T-GRID-01 | grid | `constructorAllocatesRequestedSizeAndFillsValue` | Constructor allocates requested size and fills value | Pass |
| T-GRID-02 | grid | `defaultConstructedGridIsEmpty` | Default constructed grid is empty | Pass |
| T-GRID-03 | grid | `negativeDimensionsThrowInvalidArgument` | Negative dimensions throw invalid argument | Pass |
| T-GRID-04 | grid | `atThrowsOutOfRangeOutsideBounds` | At throws out of range outside bounds | Pass |
| T-GRID-05 | grid | `rowPointersAddressTheSameElementsAsAt` | Row pointers address the same elements as at | Pass |
| T-GRID-06 | grid | `copyIsDeepAndIndependent` | Copy is deep and independent | Pass |
| T-GRID-07 | grid | `selfAssignmentKeepsContents` | Self assignment keeps contents | Pass |
| T-GRID-08 | grid | `moveTransfersOwnershipAndEmptiesSource` | Move transfers ownership and empties source | Pass |
| T-GRID-09 | grid | `equalityComparesDimensionsAndValues` | Equality compares dimensions and values | Pass |
| T-GRID-10 | grid | `worksWithBoolElements` | Works with bool elements | Pass |
| T-MOVEMENT-01 | movement | `levelStartsOnStartCellWithTimerStopped` | Level starts on start cell with timer stopped | Pass |
| T-MOVEMENT-02 | movement | `wallsBlockPlayerMoves` | Walls block player moves | Pass |
| T-MOVEMENT-03 | movement | `timerStartsOnFirstSuccessfulMove` | Timer starts on first successful move | Pass |
| T-MOVEMENT-04 | movement | `bufferedMoveRunsWhenCurrentMoveEnds` | Buffered move runs when current move ends | Pass |
| T-MOVEMENT-05 | movement | `turningChangesFacingOnly` | Turning changes facing only | Pass |
| T-PLAYTHROUGH-01 | playthrough | `solverFollowingBfsPathCompletesLevelWithStars` | Solver following BFS path completes level with stars | Pass |
| T-PLAYTHROUGH-02 | playthrough | `slowPlayerEarnsFewerStars` | Slow player earns fewer stars | Pass |
| T-GENERATION-12 | generation | `levelGenerationIsDeterministicForSeedLevelAndSize` | Level generation is deterministic for seed level and size | Pass |
| T-MOVEMENT-06 | movement | `wallsBlockNormalMovement` | Walls block normal movement | Pass |
| T-MOVEMENT-07 | movement | `openPassagesAllowMovementBothWays` | Open passages allow movement both ways | Pass |
| T-MOVEMENT-08 | movement | `secretDoorsAllowOnlyMoversThatUseThem` | Secret doors allow only movers that use them | Pass |
| T-MOVEMENT-09 | movement | `secretDoorsCannotBePlacedOnBoundaryOrOpenings` | Secret doors cannot be placed on boundary or openings | Pass |
| T-MOVEMENT-10 | movement | `phaseAllowsInnerWallsButNeverTheBoundary` | Phase allows inner walls but never the boundary | Pass |
| T-MOVEMENT-11 | movement | `addWallRemovesSecretDoorFlags` | Add wall removes secret door flags | Pass |
| T-MOVEMENT-12 | movement | `outOfBoundsCellsCannotMove` | Out of bounds cells cannot move | Pass |
| T-PATHFINDING-01 | pathfinding | `bfsFindsKnownShortestPathInSerpentine` | BFS finds known shortest path in serpentine | Pass |
| T-PATHFINDING-02 | pathfinding | `bfsFindsManhattanPathInOpenField` | BFS finds manhattan path in open field | Pass |
| T-PATHFINDING-03 | pathfinding | `pathLengthMatchesDistanceMap` | Path length matches distance map | Pass |
| T-PATHFINDING-04 | pathfinding | `unreachableTargetsGiveEmptyPath` | Unreachable targets give empty path | Pass |
| T-PATHFINDING-05 | pathfinding | `nextStepAlwaysReducesDistanceByOne` | Next step always reduces distance by one | Pass |
| T-PATHFINDING-06 | pathfinding | `pathToSelfIsSingleCell` | Path to self is single cell | Pass |
| T-PERKS-01 | perks | `spawnRulesKeepStartAreaAndExitFree` | Spawn rules keep start area and exit free | Pass |
| T-PERKS-02 | perks | `perkCountScalesWithSizeAndTier` | Perk count scales with size and tier | Pass |
| T-PERKS-03 | perks | `tierSpecificPerksOnlyAppearInTheirTiers` | Tier specific perks only appear in their tiers | Pass |
| T-PERKS-04 | perks | `magicBerryIsStoredAndPhasesThroughInnerWalls` | Magic berry is stored and phases through inner walls | Pass |
| T-PERKS-05 | perks | `phaseNeverCrossesTheBoundaryAndEndsOnAValidCell` | Phase never crosses the boundary and ends on a valid cell | Pass |
| T-PERKS-06 | perks | `fullInventoryLeavesBerryOnTheFloor` | Full inventory leaves berry on the floor | Pass |
| T-PERKS-07 | perks | `hourglassFreezesTheTimer` | Hourglass freezes the timer | Pass |
| T-PERKS-08 | perks | `compassSparkShowsTheShortestPathToTheExit` | Compass spark shows the shortest path to the exit | Pass |
| T-PERKS-09 | perks | `lanternDoublesTheFogRadius` | Lantern doubles the fog radius | Pass |
| T-PERKS-10 | perks | `frostShardFreezesTheChaser` | Frost shard freezes the chaser | Pass |
| T-PERKS-11 | perks | `collectedPerksAndEffectsSurviveSnapshots` | Collected perks and effects survive snapshots | Pass |
| T-UNLOCKS-01 | unlocks | `newProgressHasOnlyLevelOneAndWisp` | New progress has only level one and wisp | Pass |
| T-UNLOCKS-02 | unlocks | `completingALevelUnlocksTheNext` | Completing a level unlocks the next | Pass |
| T-UNLOCKS-03 | unlocks | `quittingNeverUnlocksAnything` | Quitting never unlocks anything | Pass |
| T-UNLOCKS-04 | unlocks | `skinsUnlockOnFirstCompletionOfMilestoneLevels` | Skins unlock on first completion of milestone levels | Pass |
| T-UNLOCKS-05 | unlocks | `skinUnlockLevelsMatchTheTable` | Skin unlock levels match the table | Pass |
| T-UNLOCKS-06 | unlocks | `lockedSkinsCannotBeSelected` | Locked skins cannot be selected | Pass |
| T-UNLOCKS-07 | unlocks | `finalLevelCompletesCampaignOnce` | Final level completes campaign once | Pass |
| T-UNLOCKS-08 | unlocks | `invalidRecordsAreRejectedOnRestore` | Invalid records are rejected on restore | Pass |
| T-SETTINGS-01 | settings | `outOfRangeValuesAreClamped` | Out of range values are clamped | Pass |
| T-SETTINGS-02 | settings | `effectiveVolumeCombinesMasterChannelAndMute` | Effective volume combines master channel and mute | Pass |
| T-SAVES-01 | saves | `serializeThenParseRoundTripsEveryField` | Serialize then parse round trips every field | Pass |
| T-SAVES-02 | saves | `garbageTextIsReportedAsCorrupt` | Garbage text is reported as corrupt | Pass |
| T-SAVES-03 | saves | `truncatedFileIsReportedAsCorrupt` | Truncated file is reported as corrupt | Pass |
| T-SAVES-04 | saves | `invalidFieldValuesAreRejected` | Invalid field values are rejected | Pass |
| T-SAVES-05 | saves | `duplicateKeysAreRejected` | Duplicate keys are rejected | Pass |
| T-SAVES-06 | saves | `inconsistentProgressIsRejected` | Inconsistent progress is rejected | Pass |
| T-SAVES-07 | saves | `slotFilesRoundTripThroughDisk` | Slot files round trip through disk | Pass |
| T-SAVES-08 | saves | `corruptedFileOnDiskIsDetectedWithoutCrashing` | Corrupted file on disk is detected without crashing | Pass |
| T-SAVES-09 | saves | `resumingASnapshotRestoresTheExactState` | Resuming a snapshot restores the exact state | Pass |
| T-SAVES-10 | saves | `snapshotForAnotherSeedIsRefused` | Snapshot for another seed is refused | Pass |
| T-SETTINGS-03 | settings | `settingsFileRoundTripsAndClampsValues` | Settings file round trips and clamps values | Pass |
| T-SETTINGS-04 | settings | `brokenSettingsFileFallsBackToDefaults` | Broken settings file falls back to defaults | Pass |
| T-STARS-01 | stars | `parTimeFollowsFormula` | Par time follows formula | Pass |
| T-STARS-02 | stars | `exactlyOnThreeStarThresholdEarnsThreeStars` | Exactly on three star threshold earns three stars | Pass |
| T-STARS-03 | stars | `justAboveThreeStarThresholdEarnsTwoStars` | Just above three star threshold earns two stars | Pass |
| T-STARS-04 | stars | `exactlyOnTwoStarThresholdEarnsTwoStars` | Exactly on two star threshold earns two stars | Pass |
| T-STARS-05 | stars | `slowerThanTwoStarThresholdEarnsOneStar` | Slower than two star threshold earns one star | Pass |
| T-STARS-06 | stars | `starTargetCountsDownToNextThreshold` | Star target counts down to next threshold | Pass |
| T-STARS-07 | stars | `quittingGivesZeroStarsAndKeepsPreviousBest` | Quitting gives zero stars and keeps previous best | Pass |
| T-STARS-08 | stars | `recordKeepsBestStarsAndBestTimeIndependently` | Record keeps best stars and best time independently | Pass |
| T-STARS-09 | stars | `millisecondConversionClampsNegativeAndHugeValues` | Millisecond conversion clamps negative and huge values | Pass |
| T-STARS-10 | stars | `durationFormatting` | Duration formatting | Pass |
| T-TEXTVIEW-01 | textview | `outputHasTwoNPlusOneRowsAndColumns` | Output has 2N plus one rows and columns | Pass |
| T-TEXTVIEW-02 | textview | `startAndExitAreMarked` | Start and exit are marked | Pass |
| T-TEXTVIEW-03 | textview | `wallCharactersMatchMazeLayout` | Wall characters match maze layout | Pass |
| T-TEXTVIEW-04 | textview | `secretDoorsAndMarkersAreDrawn` | Secret doors and markers are drawn | Pass |
| T-TEXTVIEW-05 | textview | `unexploredCellsAreHidden` | Unexplored cells are hidden | Pass |
| T-VALIDATION-01 | validation | `sizeNineIsRejected` | Size nine is rejected | Pass |
| T-VALIDATION-02 | validation | `sizeTenIsAccepted` | Size ten is accepted | Pass |
| T-VALIDATION-03 | validation | `sizeFiftyIsAccepted` | Size fifty is accepted | Pass |
| T-VALIDATION-04 | validation | `sizeFiftyOneIsRejected` | Size fifty one is rejected | Pass |
| T-VALIDATION-05 | validation | `negativeSizesAreRejected` | Negative sizes are rejected | Pass |
| T-VALIDATION-06 | validation | `nonNumericSizesAreRejectedWithMessage` | Non numeric sizes are rejected with message | Pass |
| T-VALIDATION-07 | validation | `surroundingSpacesAndPlusSignAreTolerated` | Surrounding spaces and plus sign are tolerated | Pass |
| T-VALIDATION-08 | validation | `integerOverflowIsRejected` | Integer overflow is rejected | Pass |
| T-VALIDATION-09 | validation | `parseIntegerAcceptsOnlyWholeNumbers` | Parse integer accepts only whole numbers | Pass |
| T-VALIDATION-10 | validation | `validNamesAreAcceptedAndTrimmed` | Valid names are accepted and trimmed | Pass |
| T-VALIDATION-11 | validation | `invalidNamesAreRejected` | Invalid names are rejected | Pass |
| T-VALIDATION-12 | validation | `slotNumbersMustBeOneToThree` | Slot numbers must be one to three | Pass |

### Requirement coverage

| Requirement | Covered by |
|---|---|
| All N × N cells reachable | T-GENERATION-01, T-TIERS-03, T-TIERS-04, T-TIERS-07 |
| Perfect maze has exactly N² − 1 passages | T-GENERATION-02, T-TIERS-02/03 |
| Start (0,0), exit (N−1, N−1) | T-GENERATION-03 |
| Same seed → same maze | T-GENERATION-04, T-GENERATION-11 (per tier), T-GENERATION-12 (whole level) |
| Size validation 9 / 10 / 50 / 51 / negative / non-numeric | T-VALIDATION-01 … 08 |
| Walls block, secret doors allow, phase allows inner walls but never the boundary | T-MOVEMENT-*, T-PERKS-04/05 |
| Every secret door shortens the path to the exit | T-TIERS-05 |
| BFS returns the correct shortest path | T-PATHFINDING-01 … 03 |
| Every chaser step reduces its distance to the player | T-CHASER-01, T-PATHFINDING-05 |
| Fog never reveals cells through walls | T-FOG-01 … 03 |
| Star boundaries at exact thresholds; quit gives 0 and keeps the best | T-STARS-02 … 07 |
| Skin unlock rules | T-UNLOCKS-04 … 06 |
| Settings clamping | T-SETTINGS-01, T-SETTINGS-03 |
| Save round trip, corrupted and truncated saves, exact snapshot resume | T-SAVES-01 … 10 |
| Text view dimensions | T-TEXTVIEW-01 |
| Simulated play-through with star computation | T-PLAYTHROUGH-01/02 |

*(IDs are numbered in the order the tests are registered within each suite; run
`maze_tests --list` to see them in that order.)*

## 4. Manual UI test checklist

Use a Release build at 1280 × 720 unless stated otherwise. Tick each item.

### Start-up and menus
- [ ] Splash appears, skips on any key and moves on by itself after about 3 s.
- [ ] Main menu shows the self-generating maze behind the buttons; text stays readable.
- [ ] Every menu works with the **mouse only** and with the **keyboard only** (arrows, Enter, Esc).
- [ ] Hover and click sounds play; a slider being dragged does not machine-gun clicks.
- [ ] *Continue* is disabled without a profile and resumes the last slot otherwise.

### New game
- [ ] Choosing an occupied slot asks for confirmation before overwriting.
- [ ] Name: an empty name, 17 characters or symbols such as `#` show an inline error; 1–16 letters, digits and spaces are accepted.
- [ ] Size: typing 9, 51, -5 or `abc` shows *"Size must be between 10 and 50."* and disables *Next*; the slider and the field stay in sync.
- [ ] Mode choice shows a description; *Start* opens Level Select.

### Level select, load and settings
- [ ] Locked levels are greyed with a lock and cannot be selected; best stars show on completed tiles.
- [ ] The view toggle switches between 2D Classic and 3D First-Person and is remembered.
- [ ] Load Game shows name, level, stars, size, mode and last-played time; *Resume Level X* appears after Save & Exit.
- [ ] Deleting a slot asks for confirmation. A hand-corrupted `slotN.sav` shows *"Save corrupted"* and can be deleted.
- [ ] Settings: volume sliders and mute act immediately; fullscreen toggles borderless mode; the FPS counter and head bob work; values survive a restart (`settings.cfg`).
- [ ] Skins: locked skins say *"Complete level X"*; unlocked skins animate and can be selected.

### 2D gameplay
- [ ] Movement is smooth; holding a key keeps walking; walls stop you with a bump sound.
- [ ] The timer starts on the first move, freezes in the pause menu and while an Hourglass runs.
- [ ] The HUD shows level, tier, timer, the next star limit, cell coordinates, berries and effect bars.
- [ ] Hovering a cell shows its coordinates and type (path, exit, secret door, perk with description).
- [ ] `T` shows the ASCII text view, which updates after each move.
- [ ] A 50 × 50 maze uses the follow camera (clamped to the maze) and runs at 60 FPS.
- [ ] Picking up a perk shows particles, a toast and plays a chime.
- [ ] Shift with a berry makes the walls translucent; you can pass inner walls but never the outer border.
- [ ] Losing window focus pauses the game.

### Tiers
- [ ] Each tier shows its intro card once per profile.
- [ ] Labyrinth: green rune doors can be walked through.
- [ ] Fog: only nearby cells are visible, explored cells stay dim, the exit beacon pulses until the exit is seen.
- [ ] Hunted: the Hunter appears about 3 s after the first move, the pink vignette and heartbeat grow as it approaches, and being caught shakes the screen and shows *Caught!*.
- [ ] A Frost Shard visibly freezes the Hunter.

### 3D mode
- [ ] W/S walk, A/D turn smoothly, Q/E strafe, Shift phases.
- [ ] Distance fog, glowing portal, rotating perks and the bobbing Hunter are visible.
- [ ] The minimap shows explored cells, the arrow follows the view, `M` toggles its size; in fog levels only explored cells are shown.
- [ ] A 50 × 50 maze runs at 60 FPS.

### Results and saving
- [ ] Level Complete shows the time, stars popping in with a ding each, the best record or *NEW RECORD!*, and *New skin unlocked!* on levels 5, 10, 15 and 20.
- [ ] *Quit Level* warns *"You will get 0 stars"* and leaves the previous best unchanged.
- [ ] Save & Exit, then Continue, resumes at the exact cell with the same time, inventory, fog and Hunter.
- [ ] Completing level 20 shows the Game Complete screen.
- [ ] Resizing the window down to 960 × 540 keeps every screen usable.

## 5. Known limitations

- Automated tests cover the pure core and persistence. Rendering, audio playback and input are
  checked through the screenshot tour and the manual checklist.
- On the development machine only MSVC was available. GCC/Clang builds use the equivalent flags
  (`-Wall -Wextra -Wpedantic -Wshadow -Werror`) but were not run there.
