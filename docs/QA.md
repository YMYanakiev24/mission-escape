# QA documentation

This document describes how Mission Escape is tested: the test strategy, the test harness, every
automated test function, the manual test checklist for the user interface and how each
requirement of the assignment is covered.

## 1. Test strategy

| Level | What is tested | How |
|---|---|---|
| Unit tests | Business logic layer: maze, generation, BFS, level rules, fog, Hunter, perks, scoring, progress, validation, profiles | `maze_tests`, no window needed |
| Unit tests | Data access layer: key=value format, save slots, settings | `maze_tests`, temporary folders on disk |
| Unit tests | Sound synthesis (`Synth`) | `maze_tests`, the synthesiser has no raylib code |
| Integration | Full level playthroughs with a BFS "player" | `playthrough` suite |
| Layer boundaries | The presentation layer cannot include the data layer | CMake include folders (see [ARCHITECTURE.md](ARCHITECTURE.md)) |
| Visual | Every screen and tier | `maze_game --screenshots` captures PNGs to `screenshots/` |
| Manual | Menus, controls, saving, display | The checklist in section 5 |

Because the business layer has no raylib and no files, every game rule is tested without opening
a window. `ProfileService` is tested with in-memory repositories (fakes of the data layer), which
is only possible because of the three-layer design.

## 2. Test harness

The project uses its own small harness, `tests/TestHarness.h`, with no external framework:

- `TEST_CASE(suite, name)` defines a test function and registers it before `main()` runs, through
  a static `Registrar` object.
- `CHECK(condition)` records a failure and continues the test.
- `CHECK_EQ(actual, expected)` also prints both values on failure.
- `REQUIRE(condition)` records a failure and stops the test (it throws `RequireFailed`).
- Unexpected exceptions are caught and reported as failures.
- `tests/test_main.cpp` runs every test, or one suite, and returns 0 only if all passed.

## 3. Running the tests

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release            # all 17 suites, one CTest entry each
build/Release/maze_tests --list              # list every test function
build/Release/maze_tests --suite saves       # run one suite
```

**Latest automated run:** 17 of 17 suites and all 138 test functions passed on 14 November 2026
(Debug build, MSVC, Visual Studio 2026, Windows 11).

## 4. Automated test functions

### Suites

| Suite | File | Tests | What it covers |
|---|---|---|---|
| grid | test_grid2d.cpp | 10 | The dynamic 2D array: allocation, bounds, deep copy, move, equality |
| generation | test_generation.cpp, test_level.cpp, test_tiers.cpp | 12 | Perfect mazes, determinism, seeds, random numbers |
| validation | test_validation.cpp | 12 | Maze size, player name and slot number input |
| movement | test_movement.cpp, test_level.cpp | 13 | Walls, secret doors, phasing, timer, buffered moves, step counter |
| pathfinding | test_pathfinding.cpp | 6 | BFS shortest paths and single steps |
| textview | test_textview.cpp | 5 | The ASCII text view |
| stars | test_scoring.cpp | 10 | Par time, star thresholds, records, time formatting |
| playthrough | test_level.cpp | 2 | Complete levels played by a BFS solver |
| unlocks | test_progress.cpp | 8 | Unlocking levels and skins, campaign completion |
| settings | test_progress.cpp, test_saves.cpp | 5 | Settings ranges, volume, settings file |
| saves | test_saves.cpp | 11 | Save files, corruption detection, Save & Exit snapshots |
| fog | test_fog.cpp | 6 | Visibility, walls blocking sight, exploration memory |
| chaser | test_chaser.cpp | 7 | The Hunter: spawning, path following, speed, catching |
| tiers | test_tiers.cpp | 7 | Tier rules: candidates, exit decoys, braiding, secret doors |
| perks | test_perks.cpp | 11 | Perk placement and every perk's effect |
| audio | test_audio.cpp | 4 | Synthesised sound effects and music loops |
| profiles | test_profiles.cpp | 9 | `ProfileService` with in-memory repositories |

### Test functions

Each test function's name says what it checks; the table spells it out.

#### grid (10)

| Test function | File | What it checks |
|---|---|---|
| `constructorAllocatesRequestedSizeAndFillsValue` | test_grid2d.cpp | Constructor allocates requested size and fills value. |
| `defaultConstructedGridIsEmpty` | test_grid2d.cpp | Default constructed grid is empty. |
| `negativeDimensionsThrowInvalidArgument` | test_grid2d.cpp | Negative dimensions throw invalid argument. |
| `atThrowsOutOfRangeOutsideBounds` | test_grid2d.cpp | `at()` throws out of range outside bounds. |
| `rowPointersAddressTheSameElementsAsAt` | test_grid2d.cpp | Row pointers address the same elements as at. |
| `copyIsDeepAndIndependent` | test_grid2d.cpp | Copy is deep and independent. |
| `selfAssignmentKeepsContents` | test_grid2d.cpp | Self assignment keeps contents. |
| `moveTransfersOwnershipAndEmptiesSource` | test_grid2d.cpp | Move transfers ownership and empties source. |
| `equalityComparesDimensionsAndValues` | test_grid2d.cpp | Equality compares dimensions and values. |
| `worksWithBoolElements` | test_grid2d.cpp | Works with bool elements. |

#### generation (12)

| Test function | File | What it checks |
|---|---|---|
| `perfectMazeReachesEveryCell` | test_generation.cpp | Perfect maze reaches every cell. |
| `perfectMazeHasExactlyNSquaredMinusOnePassages` | test_generation.cpp | Perfect maze has exactly N² − 1 passages. |
| `startIsTopLeftAndExitIsBottomRight` | test_generation.cpp | Start is top left and exit is bottom right. |
| `sameSeedProducesIdenticalMaze` | test_generation.cpp | Same seed produces identical maze. |
| `differentSeedsProduceDifferentMazes` | test_generation.cpp | Different seeds produce different mazes. |
| `boundaryWallsAreNeverCarved` | test_generation.cpp | Boundary walls are never carved. |
| `builderStepsVisitEveryCellBeforeFinishing` | test_generation.cpp | Builder steps visit every cell before finishing. |
| `wallsStayConsistentOnBothSides` | test_generation.cpp | Walls stay consistent on both sides. |
| `derivedLevelSeedsDifferPerLevelAndSize` | test_generation.cpp | Derived level seeds differ per level and size. |
| `rngNextIntStaysWithinInclusiveBounds` | test_generation.cpp | `Rng::nextInt` stays within inclusive bounds. |
| `levelGenerationIsDeterministicForSeedLevelAndSize` | test_level.cpp | Level generation is deterministic for seed level and size. |
| `levelMazesAreDeterministicPerTier` | test_tiers.cpp | Level mazes are deterministic per tier. |

#### validation (12)

| Test function | File | What it checks |
|---|---|---|
| `sizeNineIsRejected` | test_validation.cpp | Size 9 is rejected. |
| `sizeTenIsAccepted` | test_validation.cpp | Size 10 is accepted. |
| `sizeFiftyIsAccepted` | test_validation.cpp | Size 50 is accepted. |
| `sizeFiftyOneIsRejected` | test_validation.cpp | Size 51 is rejected. |
| `negativeSizesAreRejected` | test_validation.cpp | Negative sizes are rejected. |
| `nonNumericSizesAreRejectedWithMessage` | test_validation.cpp | Non numeric sizes are rejected with message. |
| `surroundingSpacesAndPlusSignAreTolerated` | test_validation.cpp | Surrounding spaces and plus sign are tolerated. |
| `integerOverflowIsRejected` | test_validation.cpp | Integer overflow is rejected. |
| `parseIntegerAcceptsOnlyWholeNumbers` | test_validation.cpp | `parseInteger` accepts only whole numbers. |
| `validNamesAreAcceptedAndTrimmed` | test_validation.cpp | Valid names are accepted and trimmed. |
| `invalidNamesAreRejected` | test_validation.cpp | Invalid names are rejected. |
| `slotNumbersMustBeOneToThree` | test_validation.cpp | Slot numbers must be one to three. |

#### movement (13)

| Test function | File | What it checks |
|---|---|---|
| `levelStartsOnStartCellWithTimerStopped` | test_level.cpp | Level starts on start cell with timer stopped. |
| `wallsBlockPlayerMoves` | test_level.cpp | Walls block player moves. |
| `timerStartsOnFirstSuccessfulMove` | test_level.cpp | Timer starts on first successful move. |
| `bufferedMoveRunsWhenCurrentMoveEnds` | test_level.cpp | Buffered move runs when current move ends. |
| `stepCounterCountsOnlyStartedMoves` | test_level.cpp | Step counter counts only started moves. |
| `turningChangesFacingOnly` | test_level.cpp | Turning changes facing only. |
| `wallsBlockNormalMovement` | test_movement.cpp | Walls block normal movement. |
| `openPassagesAllowMovementBothWays` | test_movement.cpp | Open passages allow movement both ways. |
| `secretDoorsAllowOnlyMoversThatUseThem` | test_movement.cpp | Secret doors allow only movers that use them. |
| `secretDoorsCannotBePlacedOnBoundaryOrOpenings` | test_movement.cpp | Secret doors cannot be placed on boundary or openings. |
| `phaseAllowsInnerWallsButNeverTheBoundary` | test_movement.cpp | Phase allows inner walls but never the boundary. |
| `addWallRemovesSecretDoorFlags` | test_movement.cpp | Add wall removes secret door flags. |
| `outOfBoundsCellsCannotMove` | test_movement.cpp | Out of bounds cells cannot move. |

#### pathfinding (6)

| Test function | File | What it checks |
|---|---|---|
| `bfsFindsKnownShortestPathInSerpentine` | test_pathfinding.cpp | BFS finds known shortest path in serpentine. |
| `bfsFindsManhattanPathInOpenField` | test_pathfinding.cpp | BFS finds Manhattan path in open field. |
| `pathLengthMatchesDistanceMap` | test_pathfinding.cpp | Path length matches distance map. |
| `unreachableTargetsGiveEmptyPath` | test_pathfinding.cpp | Unreachable targets give empty path. |
| `nextStepAlwaysReducesDistanceByOne` | test_pathfinding.cpp | Next step always reduces distance by one. |
| `pathToSelfIsSingleCell` | test_pathfinding.cpp | Path to self is single cell. |

#### textview (5)

| Test function | File | What it checks |
|---|---|---|
| `outputHasTwoNPlusOneRowsAndColumns` | test_textview.cpp | Output has 2N + 1 rows and columns. |
| `startAndExitAreMarked` | test_textview.cpp | Start and exit are marked. |
| `wallCharactersMatchMazeLayout` | test_textview.cpp | Wall characters match maze layout. |
| `secretDoorsAndMarkersAreDrawn` | test_textview.cpp | Secret doors and markers are drawn. |
| `unexploredCellsAreHidden` | test_textview.cpp | Unexplored cells are hidden. |

#### stars (10)

| Test function | File | What it checks |
|---|---|---|
| `parTimeFollowsFormula` | test_scoring.cpp | Par time follows formula. |
| `exactlyOnThreeStarThresholdEarnsThreeStars` | test_scoring.cpp | Exactly on 3-star threshold earns three stars. |
| `justAboveThreeStarThresholdEarnsTwoStars` | test_scoring.cpp | Just above 3-star threshold earns two stars. |
| `exactlyOnTwoStarThresholdEarnsTwoStars` | test_scoring.cpp | Exactly on 2-star threshold earns two stars. |
| `slowerThanTwoStarThresholdEarnsOneStar` | test_scoring.cpp | Slower than 2-star threshold earns 1 star. |
| `starTargetCountsDownToNextThreshold` | test_scoring.cpp | Star target counts down to next threshold. |
| `quittingGivesZeroStarsAndKeepsPreviousBest` | test_scoring.cpp | Quitting gives zero stars and keeps previous best. |
| `recordKeepsBestStarsAndBestTimeIndependently` | test_scoring.cpp | Record keeps best stars and best time independently. |
| `millisecondConversionClampsNegativeAndHugeValues` | test_scoring.cpp | Millisecond conversion clamps negative and huge values. |
| `durationFormatting` | test_scoring.cpp | Duration formatting. |

#### playthrough (2)

| Test function | File | What it checks |
|---|---|---|
| `solverFollowingBfsPathCompletesLevelWithStars` | test_level.cpp | Solver following BFS path completes level with stars. |
| `slowPlayerEarnsFewerStars` | test_level.cpp | Slow player earns fewer stars. |

#### unlocks (8)

| Test function | File | What it checks |
|---|---|---|
| `newProgressHasOnlyLevelOneAndWisp` | test_progress.cpp | New progress has only level 1 and Wisp. |
| `completingALevelUnlocksTheNext` | test_progress.cpp | Completing a level unlocks the next. |
| `quittingNeverUnlocksAnything` | test_progress.cpp | Quitting never unlocks anything. |
| `skinsUnlockOnFirstCompletionOfMilestoneLevels` | test_progress.cpp | Skins unlock on first completion of milestone levels. |
| `skinUnlockLevelsMatchTheTable` | test_progress.cpp | Skin unlock levels match the table. |
| `lockedSkinsCannotBeSelected` | test_progress.cpp | Locked skins cannot be selected. |
| `finalLevelCompletesCampaignOnce` | test_progress.cpp | Final level completes campaign once. |
| `invalidRecordsAreRejectedOnRestore` | test_progress.cpp | Invalid records are rejected on restore. |

#### settings (5)

| Test function | File | What it checks |
|---|---|---|
| `outOfRangeValuesAreClamped` | test_progress.cpp | Out of range values are clamped. |
| `effectiveVolumeCombinesMasterChannelAndMute` | test_progress.cpp | Effective volume combines master channel and mute. |
| `settingsFileRoundTripsAndClampsValues` | test_saves.cpp | Settings file round trips and clamps values. |
| `screenShakeIsSavedAndDefaultsToOn` | test_saves.cpp | Screen shake is saved and defaults to on. |
| `brokenSettingsFileFallsBackToDefaults` | test_saves.cpp | Broken settings file falls back to defaults. |

#### saves (11)

| Test function | File | What it checks |
|---|---|---|
| `serializeThenParseRoundTripsEveryField` | test_saves.cpp | Serialize then parse round trips every field. |
| `garbageTextIsReportedAsCorrupt` | test_saves.cpp | Garbage text is reported as corrupt. |
| `truncatedFileIsReportedAsCorrupt` | test_saves.cpp | Truncated file is reported as corrupt. |
| `invalidFieldValuesAreRejected` | test_saves.cpp | Invalid field values are rejected. |
| `duplicateKeysAreRejected` | test_saves.cpp | Duplicate keys are rejected. |
| `inconsistentProgressIsRejected` | test_saves.cpp | Inconsistent progress is rejected. |
| `slotFilesRoundTripThroughDisk` | test_saves.cpp | Slot files round trip through disk. |
| `corruptedFileOnDiskIsDetectedWithoutCrashing` | test_saves.cpp | Corrupted file on disk is detected without crashing. |
| `resumingASnapshotRestoresTheExactState` | test_saves.cpp | Resuming a snapshot restores the exact state. |
| `snapshotsWithoutAStepCountStillLoad` | test_saves.cpp | Snapshots without a step count still load. |
| `snapshotForAnotherSeedIsRefused` | test_saves.cpp | Snapshot for another seed is refused. |

#### fog (6)

| Test function | File | What it checks |
|---|---|---|
| `visibleCellsAreAlwaysWithinRadiusThroughOpenPassages` | test_fog.cpp | Visible cells are always within radius through open passages. |
| `wallsBlockSightEvenToAdjacentCells` | test_fog.cpp | Walls block sight even to adjacent cells. |
| `secretDoorsBlockSight` | test_fog.cpp | Secret doors block sight. |
| `exploredCellsAreRememberedButNotVisible` | test_fog.cpp | Explored cells are remembered but not visible. |
| `encodedExplorationRoundTrips` | test_fog.cpp | Encoded exploration round trips. |
| `fogLevelsRevealOnlyNearThePlayer` | test_fog.cpp | Fog levels reveal only near the player. |

#### chaser (7)

| Test function | File | What it checks |
|---|---|---|
| `everyStepReducesDistanceToPlayer` | test_chaser.cpp | Every step reduces distance to player. |
| `neverUsesSecretDoors` | test_chaser.cpp | Never uses secret doors. |
| `frozenChaserDoesNotMove` | test_chaser.cpp | Frozen chaser does not move. |
| `spawnsAtLeastHalfTheMazeAwayAndNotNearTheExit` | test_chaser.cpp | Spawns at least half the maze away and not near the exit. |
| `speedRampsUpButIsCapped` | test_chaser.cpp | Speed ramps up but is capped. |
| `appearsAfterGracePeriodAndCatchesAnIdlePlayer` | test_chaser.cpp | Appears after grace period and catches an idle player. |
| `dangerRisesAsTheChaserApproaches` | test_chaser.cpp | Danger rises as the chaser approaches. |

#### tiers (7)

| Test function | File | What it checks |
|---|---|---|
| `tierBoundariesMatchTheCampaignTable` | test_tiers.cpp | Tier boundaries match the campaign table. |
| `hardestCandidateScoresAtLeastAsHighAsASingleMaze` | test_tiers.cpp | Hardest candidate scores at least as high as a single maze. |
| `exitDecoysKeepTheMazePerfectAndThePathUnchanged` | test_tiers.cpp | Exit decoys keep the maze perfect and the path unchanged. |
| `braidingRemovesDeadEndsAndCreatesLoops` | test_tiers.cpp | Braiding removes dead ends and creates loops. |
| `everySecretDoorShortensThePathToTheExit` | test_tiers.cpp | Every secret door shortens the path to the exit. |
| `awakeningLevelsHaveNoSecretDoors` | test_tiers.cpp | Awakening levels have no secret doors. |
| `labyrinthLevelsStayPerfectApartFromDoors` | test_tiers.cpp | Labyrinth levels stay perfect apart from doors. |

#### perks (11)

| Test function | File | What it checks |
|---|---|---|
| `spawnRulesKeepStartAreaAndExitFree` | test_perks.cpp | Spawn rules keep start area and exit free. |
| `perkCountScalesWithSizeAndTier` | test_perks.cpp | Perk count scales with size and tier. |
| `tierSpecificPerksOnlyAppearInTheirTiers` | test_perks.cpp | Tier specific perks only appear in their tiers. |
| `magicBerryIsStoredAndPhasesThroughInnerWalls` | test_perks.cpp | Magic berry is stored and phases through inner walls. |
| `phaseNeverCrossesTheBoundaryAndEndsOnAValidCell` | test_perks.cpp | Phase never crosses the boundary and ends on a valid cell. |
| `fullInventoryLeavesBerryOnTheFloor` | test_perks.cpp | Full inventory leaves berry on the floor. |
| `hourglassFreezesTheTimer` | test_perks.cpp | Hourglass freezes the timer. |
| `compassSparkShowsTheShortestPathToTheExit` | test_perks.cpp | Compass spark shows the shortest path to the exit. |
| `lanternDoublesTheFogRadius` | test_perks.cpp | Lantern doubles the fog radius. |
| `frostShardFreezesTheChaser` | test_perks.cpp | Frost shard freezes the chaser. |
| `collectedPerksAndEffectsSurviveSnapshots` | test_perks.cpp | Collected perks and effects survive snapshots. |

#### audio (4)

| Test function | File | What it checks |
|---|---|---|
| `everySoundEffectIsAudibleAndNeverClips` | test_audio.cpp | Every sound effect is audible and never clips. |
| `soundEffectsStartAndEndSilentlyToAvoidClicks` | test_audio.cpp | Sound effects start and end silently to avoid clicks. |
| `musicLoopsHaveExactLengthAndJoinSeamlessly` | test_audio.cpp | Music loops have exact length and join seamlessly. |
| `synthesisIsDeterministic` | test_audio.cpp | Synthesis is deterministic. |

#### profiles (9)

| Test function | File | What it checks |
|---|---|---|
| `loadingReportsTheStateOfEverySlot` | test_profiles.cpp | Loading reports the state of every slot. |
| `creatingAProfileActivatesAndSavesIt` | test_profiles.cpp | Creating a profile activates and saves it. |
| `onlyReadySlotsCanBeActivated` | test_profiles.cpp | Only ready slots can be activated. |
| `recordingAResultUpdatesProgressAndDropsTheSavedAttempt` | test_profiles.cpp | Recording a result updates progress and drops the saved attempt. |
| `recordingWithoutAnActiveProfileChangesNothing` | test_profiles.cpp | Recording without an active profile changes nothing. |
| `saveAndExitStoresTheAttemptAndADiscardRemovesIt` | test_profiles.cpp | Save and exit stores the attempt and a discard removes it. |
| `deletingTheActiveSlotForgetsIt` | test_profiles.cpp | Deleting the active slot forgets it. |
| `aFailedSaveIsReportedToTheUser` | test_profiles.cpp | A failed save is reported to the user. |
| `disabledPersistenceNeverTouchesStorage` | test_profiles.cpp | Disabled persistence never touches storage. |

## 5. Manual test checklist

The user interface is checked by hand on a Release build. Record the result (Pass/Fail), the date
and the tester for every row, and open a GitHub issue for every failure.

| ID | Area | Steps | Expected result | Result |
|---|---|---|---|---|
| M01 | Start-up | Start `maze_game` | The splash screen appears, then the main menu | |
| M02 | New game | New Game → enter maze size 9 | "Size must be between 10 and 50." and Next stays blocked | |
| M03 | New game | Enter maze size 51, then `abc` | Both are rejected with a message | |
| M04 | New game | Enter maze size 10 and 50 | Both are accepted | |
| M05 | New game | Enter an empty name, a 17-character name and `Ana!` | Each is rejected with a message | |
| M06 | New game | Finish the wizard with name, size 15 and 2D | Level Select opens with only level 1 unlocked | |
| M07 | Maze | Start level 1 | Player at the top-left cell, exit at the bottom-right cell, walls visible | |
| M08 | Movement | Use the arrow keys and WASD | The player moves one cell per press and keeps walking while held | |
| M09 | Movement | Walk into a wall | The player does not move and a bump sound plays | |
| M10 | Position | Move several times | The maze and the HUD cell show the new position after every move | |
| M11 | Text view | Press T | The ASCII maze appears with `@` at the player's position; T hides it | |
| M12 | Tooltip | Hover a cell with the mouse (2D) | A tooltip names the cell without coordinates | |
| M13 | Timer | Wait before the first move, then move | The timer starts only with the first move | |
| M14 | Pause | Press Esc during a level | The pause menu opens and the timer stops | |
| M15 | Finish | Reach the exit | Level Complete shows time, stars, best result and steps; level 2 unlocks | |
| M16 | Secret door | In level 6 or later, walk through a green rune | The player passes; the route is shorter | |
| M17 | Perks | Collect a Magic Berry and press Shift | The player can pass inner walls for 3 s, never the outer wall | |
| M18 | Fog | Play a Fog level (11–15) | Only nearby cells are visible; explored cells stay dim | |
| M19 | Hunter | Play a Hunted level (16–20), make one move, then stand still | The Hunter appears 3 s after the first move and catches the player; Caught screen with 0 stars | |
| M20 | Save & Exit | Pause → Save & Exit, then Continue | The level resumes at the same cell with the same time | |
| M21 | Quit level | Pause → Quit Level → confirm | 0 stars; the previous best is unchanged | |
| M22 | Restart | Press R after the first move | The same maze starts over with a fresh timer | |
| M23 | 3D view | Switch to 3D in Level Select and play | W/S move, A/D turn, Q/E strafe, M resizes the minimap | |
| M24 | Load game | Load Game → load, then delete a slot | The profile loads; after deleting, the slot shows Empty | |
| M25 | Corrupt save | Edit a `saves/slotN.sav` file by hand, restart the game | The slot shows "Save corrupted" and can be deleted; no crash | |
| M26 | Settings | Change volume, fullscreen and screen shake, restart the game | The settings are kept | |
| M27 | New maze | Start the same level twice from Level Select | The two mazes are different | |
| M28 | Window | Resize to 960 × 540 and switch to fullscreen | Every screen stays readable | |
| M29 | Gamepad | Connect a gamepad and play a level | D-pad/stick move, A selects, Start pauses; hints show gamepad buttons | |
| M30 | Exit | Main Menu → Quit | The game closes without errors | |

## 6. Requirement traceability

| # | Assignment requirement | Implementation | Automated tests | Manual |
|---|---|---|---|---|
| 1 | A maze game the player must get through to win | `Level`, `LevelState::Won` | playthrough.solverFollowingBfsPathCompletesLevelWithStars | M15 |
| 2 | Dynamic arrays; size 10×10 to 50×50 chosen at the start | `Grid2D` (`new[]`/`delete[]`), `validateMazeSize`, New Game | grid.*, validation.sizeTenIsAccepted, validation.sizeFiftyIsAccepted, validation.sizeNineIsRejected, validation.sizeFiftyOneIsRejected | M02–M04 |
| 3 | Start at (0, 0) top-left; exit bottom-right | `Maze::start()`, `Maze::exit()` | generation.startIsTopLeftAndExitIsBottomRight, movement.levelStartsOnStartCellWithTimerStopped | M07 |
| 4 | The maze is shown with the current position after every move | `Renderer2D`, `Renderer3D`, HUD | movement.timerStartsOnFirstSuccessfulMove | M10 |
| 5 | A text view of the maze from above | `mazeToText`, Text View (T) | textview.* | M11 |
| 6 | Walls, openings and the exit are visible | renderers, `mazeToText` | textview.wallCharactersMatchMazeLayout, textview.startAndExitAreMarked | M07 |
| 7 | A new maze every time | `makeRandomSeed` per attempt | generation.differentSeedsProduceDifferentMazes | M27 |
| 8 | Arrow keys; no moving through walls except a special door that shortens the way | `Input`, `Maze::canPass`, `placeSecretDoors` | movement.wallsBlockPlayerMoves, movement.secretDoorsAllowOnlyMoversThatUseThem, tiers.everySecretDoorShortensThePathToTheExit | M08, M09, M16 |
| 9 | Functions that maintain and draw the maze | `Maze`, `MazeGenerator`, renderers | generation.*, tiers.* | M07 |
| 10 | Game-logic functions do not use standard input/output | business layer has no I/O; no `cin`/`cout` in `src` | enforced by the build (the business layer has no raylib or I/O includes) | – |
| 11 | A menu | Main Menu, pause menu | – | M01, M14 |
| 12 | Validation of user input | `Validation.cpp`, save-file checks | validation.*, saves.invalidFieldValuesAreRejected, saves.garbageTextIsReportedAsCorrupt | M02–M05, M25 |

**Note on requirement 8:** the Magic Berry perk lets the player pass through inner walls for
3 seconds (never the outer wall). It is a collectible power-up, separate from the secret doors.
