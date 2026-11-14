#pragma once

/// @file Config.h
/// Every gameplay tuning value lives here, so balancing never requires hunting for magic
/// numbers in the logic. Rendering-only constants live next to the renderer that uses them.

namespace core {

// ---------------------------------------------------------------------------------------------
// Maze dimensions (assignment requirement: N x N with 10 <= N <= 50)
// ---------------------------------------------------------------------------------------------
inline constexpr int MIN_MAZE_SIZE = 10;
inline constexpr int MAX_MAZE_SIZE = 50;
inline constexpr int DEFAULT_MAZE_SIZE = 15;

// ---------------------------------------------------------------------------------------------
// Campaign structure
// ---------------------------------------------------------------------------------------------
inline constexpr int LEVEL_COUNT = 20;
inline constexpr int LEVELS_PER_TIER = 5;
inline constexpr int TIER_COUNT = 4;
inline constexpr int SAVE_SLOT_COUNT = 3;
inline constexpr int PLAYER_NAME_MAX_LENGTH = 16;

// ---------------------------------------------------------------------------------------------
// Movement (seconds needed to travel one cell)
// ---------------------------------------------------------------------------------------------
inline constexpr float MOVE_SECONDS_2D = 0.12f;
inline constexpr float MOVE_SECONDS_3D = 0.24f;

// ---------------------------------------------------------------------------------------------
// Maze generation
// ---------------------------------------------------------------------------------------------
/// Tier 2 keeps the hardest of this many candidate mazes.
inline constexpr int LABYRINTH_CANDIDATE_COUNT = 8;
/// Candidate score = path length * PATH_WEIGHT + dead ends * DEAD_END_WEIGHT.
inline constexpr int CANDIDATE_PATH_WEIGHT = 3;
inline constexpr int CANDIDATE_DEAD_END_WEIGHT = 1;
/// Number of tree-preserving edge swaps that hang dead-end branches next to the exit.
inline constexpr int EXIT_DECOY_COUNT = 3;
/// Decoys attach to one of the last cells of the solution path (excluding the exit).
inline constexpr int EXIT_DECOY_PATH_WINDOW = 12;
/// A branch must contain at least this many cells to be worth turning into a decoy.
inline constexpr int EXIT_DECOY_MIN_BRANCH_CELLS = 4;
/// Fraction of dead ends opened up in the braided tier 4 mazes.
inline constexpr float BRAID_RATIO = 0.4f;
inline constexpr int SECRET_DOOR_MIN_COUNT = 1;
inline constexpr int SECRET_DOOR_MAX_COUNT = 3;
/// A secret door must shorten the start-to-exit path by at least this many steps.
inline constexpr int SECRET_DOOR_MIN_SAVING = 4;

// ---------------------------------------------------------------------------------------------
// Fog of war and exploration
// ---------------------------------------------------------------------------------------------
inline constexpr int FOG_RADIUS = 3;
inline constexpr int LANTERN_RADIUS_MULTIPLIER = 2;
/// Exploration radius used for the 3D minimap in tiers without fog.
inline constexpr int CLEAR_EXPLORE_RADIUS = 2;
/// How far the player "sees" down straight corridors in tiers without fog (3D minimap).
inline constexpr int CORRIDOR_SIGHT_CELLS = 10;

// ---------------------------------------------------------------------------------------------
// Chaser (tier 4)
// ---------------------------------------------------------------------------------------------
inline constexpr float CHASER_GRACE_SECONDS = 3.0f;
inline constexpr float CHASER_SPEED_FIRST_LEVEL = 0.70f;
inline constexpr float CHASER_SPEED_LAST_LEVEL = 0.85f;
inline constexpr float CHASER_SPEED_RAMP_PER_SECOND = 0.002f;
inline constexpr float CHASER_SPEED_MAX = 0.95f;
/// In 3D the player must turn and sees less, so the chaser is a little slower there.
inline constexpr float CHASER_SPEED_3D_FACTOR = 0.9f;
/// Interpolated distance (in cells) below which the chaser catches the player.
inline constexpr float CHASER_CATCH_DISTANCE = 0.55f;
/// Path distance at which the danger warning starts to fade in.
inline constexpr int CHASER_WARNING_DISTANCE = 12;
/// The chaser never spawns this close (in path steps) to the exit.
inline constexpr int CHASER_EXIT_CLEARANCE = 4;

// ---------------------------------------------------------------------------------------------
// Perks and effects
// ---------------------------------------------------------------------------------------------
inline constexpr int BERRY_CAPACITY = 3;
inline constexpr float PHASE_SECONDS = 3.0f;
inline constexpr float HOURGLASS_SECONDS = 5.0f;
inline constexpr float COMPASS_SECONDS = 3.0f;
inline constexpr float LANTERN_SECONDS = 10.0f;
inline constexpr float FROST_SECONDS = 4.0f;
/// Roughly one perk per this many cells of maze width.
inline constexpr int PERK_SIZE_DIVISOR = 8;
inline constexpr int PERK_BONUS_AWAKENING = 1;
inline constexpr int PERK_BONUS_LABYRINTH = 1;
inline constexpr int PERK_BONUS_FOG = 2;
inline constexpr int PERK_BONUS_HUNTED = 2;

// ---------------------------------------------------------------------------------------------
// Timer and star rating
// ---------------------------------------------------------------------------------------------
inline constexpr float SECONDS_PER_CELL = 0.30f;
inline constexpr float THREE_STAR_FACTOR = 2.0f;
inline constexpr float TWO_STAR_FACTOR = 3.5f;
inline constexpr float TIME_MULTIPLIER_AWAKENING = 1.0f;
inline constexpr float TIME_MULTIPLIER_LABYRINTH = 0.85f;
inline constexpr float TIME_MULTIPLIER_FOG = 1.5f;
inline constexpr float TIME_MULTIPLIER_HUNTED = 1.0f;
inline constexpr float TIME_MULTIPLIER_2D = 1.0f;
inline constexpr float TIME_MULTIPLIER_3D = 1.75f;
inline constexpr int MAX_STARS = 3;
/// Stars awarded when a level is quit; never overwrites a previous best.
inline constexpr int QUIT_STARS = 0;
/// Upper bound accepted for stored times (99 hours), guards against corrupted saves.
inline constexpr int MAX_RECORD_MILLISECONDS = 99 * 60 * 60 * 1000;

// ---------------------------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------------------------
inline constexpr int SAVE_FORMAT_VERSION = 1;
inline constexpr int SETTINGS_FORMAT_VERSION = 1;
/// Files larger than this are rejected as corrupt before parsing.
inline constexpr long long MAX_SAVE_FILE_BYTES = 64 * 1024;

// ---------------------------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------------------------
inline constexpr int VOLUME_MIN = 0;
inline constexpr int VOLUME_MAX = 100;
inline constexpr int DEFAULT_MASTER_VOLUME = 80;
inline constexpr int DEFAULT_MUSIC_VOLUME = 55;
inline constexpr int DEFAULT_SFX_VOLUME = 80;

} // namespace core
