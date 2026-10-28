#include "TestHarness.h"
#include "TestUtil.h"

#include "core/Config.h"
#include "core/Level.h"
#include "core/MazeGenerator.h"
#include "core/Pathfinding.h"

#include <cstdint>

using namespace core;

namespace {

constexpr std::uint32_t SEEDS[] = {3U, 17U, 2024U, 77777U};

/// Door-free start-to-exit path length when only @p door is opened.
int pathLengthWithSingleDoor(const Maze& maze, const std::vector<SecretDoor>& doors,
                             const SecretDoor& door) {
    Maze copy = maze;
    for (const SecretDoor& other : doors) {
        if (other.cell != door.cell || other.side != door.side) {
            copy.addWall(other.cell, other.side); // turn every other door back into a wall
        }
    }
    return shortestPathLength(copy, copy.start(), copy.exit(), PassRules{true, false});
}

} // namespace

TEST_CASE(tiers, tierBoundariesMatchTheCampaignTable) {
    CHECK(tierForLevel(1) == Tier::Awakening);
    CHECK(tierForLevel(5) == Tier::Awakening);
    CHECK(tierForLevel(6) == Tier::Labyrinth);
    CHECK(tierForLevel(11) == Tier::Fog);
    CHECK(tierForLevel(16) == Tier::Hunted);
    CHECK(tierForLevel(20) == Tier::Hunted);
    CHECK(levelSpec(7).candidateCount == LABYRINTH_CANDIDATE_COUNT);
    CHECK(levelSpec(12).hasFog);
    CHECK(levelSpec(18).hasChaser);
    CHECK_EQ(levelSpec(16).chaserBaseSpeed, CHASER_SPEED_FIRST_LEVEL);
    CHECK_EQ(levelSpec(20).chaserBaseSpeed, CHASER_SPEED_LAST_LEVEL);
    CHECK_EQ(levelSpec(3).secretDoorMax, 0);
}

TEST_CASE(tiers, hardestCandidateScoresAtLeastAsHighAsASingleMaze) {
    for (const std::uint32_t seed : SEEDS) {
        Rng single(seed);
        Rng multiple(seed);
        const Maze first = generatePerfectMaze(20, single);
        const Maze best = generateHardestCandidate(20, LABYRINTH_CANDIDATE_COUNT, multiple);
        CHECK(candidateScore(best) >= candidateScore(first));
        CHECK_EQ(best.passageCount(), 20 * 20 - 1);
    }
}

TEST_CASE(tiers, exitDecoysKeepTheMazePerfectAndThePathUnchanged) {
    for (const std::uint32_t seed : SEEDS) {
        Rng rng(seed);
        Maze maze = generatePerfectMaze(25, rng);
        const int before = shortestPathLength(maze, maze.start(), maze.exit(), PassRules{});
        const int added = addExitDecoys(maze, EXIT_DECOY_COUNT, rng);
        CHECK(added > 0);
        CHECK_EQ(maze.passageCount(), 25 * 25 - 1);
        CHECK(allCellsReachable(maze, PassRules{}));
        CHECK_EQ(shortestPathLength(maze, maze.start(), maze.exit(), PassRules{}), before);
    }
}

TEST_CASE(tiers, braidingRemovesDeadEndsAndCreatesLoops) {
    for (const std::uint32_t seed : SEEDS) {
        Rng rng(seed);
        Maze maze = generatePerfectMaze(30, rng);
        const int deadEndsBefore = maze.deadEndCount();
        const int opened = braidMaze(maze, BRAID_RATIO, rng);
        CHECK(opened > 0);
        CHECK(maze.deadEndCount() <= deadEndsBefore - opened);
        CHECK(maze.passageCount() > 30 * 30 - 1); // extra passages mean loops
        CHECK(allCellsReachable(maze, PassRules{}));
    }
}

TEST_CASE(tiers, everySecretDoorShortensThePathToTheExit) {
    for (const std::uint32_t seed : SEEDS) {
        for (const int level : {6, 12, 17}) {
            Rng rng(seed + static_cast<std::uint32_t>(level));
            const GeneratedMaze generated = generateLevelMaze(levelSpec(level), 24, rng);
            const Maze& maze = generated.maze;
            const int doorFree = shortestPathLength(maze, maze.start(), maze.exit(), PassRules{});
            CHECK(static_cast<int>(generated.secretDoors.size()) <= SECRET_DOOR_MAX_COUNT);
            for (const SecretDoor& door : generated.secretDoors) {
                CHECK(maze.hasSecretDoor(door.cell, door.side));
                CHECK(!maze.isBoundary(door.cell, door.side));
                const int withDoor = pathLengthWithSingleDoor(maze, generated.secretDoors, door);
                CHECK(withDoor <= doorFree - SECRET_DOOR_MIN_SAVING);
            }
        }
    }
}

TEST_CASE(tiers, awakeningLevelsHaveNoSecretDoors) {
    Rng rng(5U);
    const GeneratedMaze generated = generateLevelMaze(levelSpec(2), 20, rng);
    CHECK(generated.secretDoors.empty());
    CHECK_EQ(generated.maze.passageCount(), 20 * 20 - 1);
}

TEST_CASE(tiers, labyrinthLevelsStayPerfectApartFromDoors) {
    const Level level(8, 20, 99U, ViewMode::Classic2D);
    CHECK_EQ(level.maze().passageCount(), 20 * 20 - 1);
    CHECK(allCellsReachable(level.maze(), PassRules{}));
}

TEST_CASE(generation, levelMazesAreDeterministicPerTier) {
    for (const int level : {1, 8, 13, 19}) {
        Rng first(31U);
        Rng second(31U);
        const GeneratedMaze a = generateLevelMaze(levelSpec(level), 21, first);
        const GeneratedMaze b = generateLevelMaze(levelSpec(level), 21, second);
        CHECK(a.maze == b.maze);
        CHECK_EQ(a.secretDoors.size(), b.secretDoors.size());
    }
}
