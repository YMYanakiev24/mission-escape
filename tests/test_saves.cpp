#include "TestHarness.h"
#include "TestUtil.h"

#include "core/Level.h"
#include "core/Pathfinding.h"
#include "io/KeyValueDocument.h"
#include "io/SaveSystem.h"
#include "io/SettingsStore.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

using namespace core;

namespace {

/// A profile with some progress and an in-progress snapshot.
SaveData sampleSave() {
    SaveData data;
    data.playerName = "Ada 2";
    data.mazeSize = 23;
    data.viewMode = ViewMode::FirstPerson3D;
    data.lastPlayed = "2026-10-18 21:40";
    data.progress.applyResult(1, 3, 41000);
    data.progress.applyResult(2, 1, 99000);
    data.progress.markTierIntroSeen(Tier::Awakening);
    LevelSnapshot snapshot;
    snapshot.level = 3;
    snapshot.seed = 4000000000U;
    snapshot.playerCell = Cell{5, 7};
    snapshot.facing = Direction::West;
    snapshot.elapsedMs = 12345;
    snapshot.timerStarted = true;
    snapshot.berries = 2;
    snapshot.collectedPerkIds = {0, 2};
    data.snapshot = snapshot;
    return data;
}

/// Replaces the value of @p key in save text.
std::string withValue(const std::string& text, const std::string& key, const std::string& value) {
    const std::size_t start = text.find(key + "=");
    if (start == std::string::npos) {
        return text;
    }
    const std::size_t end = text.find('\n', start);
    return text.substr(0, start) + key + "=" + value + text.substr(end);
}

/// A unique scratch folder under the system temp directory, removed on destruction.
class ScratchFolder {
public:
    ScratchFolder() : m_path(std::filesystem::temp_directory_path() / "mission_escape_tests") {
        std::error_code code;
        std::filesystem::remove_all(m_path, code);
    }
    ~ScratchFolder() {
        std::error_code code;
        std::filesystem::remove_all(m_path, code);
    }
    ScratchFolder(const ScratchFolder&) = delete;
    ScratchFolder& operator=(const ScratchFolder&) = delete;
    const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

} // namespace

TEST_CASE(saves, serializeThenParseRoundTripsEveryField) {
    const SaveData original = sampleSave();
    const io::LoadResult loaded = io::parseSave(io::serializeSave(original));
    REQUIRE(loaded.status == io::LoadStatus::Ok);
    const SaveData& data = loaded.data;
    CHECK_EQ(data.playerName, original.playerName);
    CHECK_EQ(data.mazeSize, 23);
    CHECK(data.viewMode == ViewMode::FirstPerson3D);
    CHECK_EQ(data.lastPlayed, original.lastPlayed);
    CHECK_EQ(data.progress.highestUnlockedLevel(), 3);
    CHECK_EQ(data.progress.record(1).bestStars, 3);
    CHECK_EQ(data.progress.record(2).bestTimeMs, 99000);
    CHECK(data.progress.hasSeenTierIntro(Tier::Awakening));
    REQUIRE(data.snapshot.has_value());
    CHECK_EQ(data.snapshot->seed, 4000000000U);
    CHECK_EQ(data.snapshot->playerCell, (Cell{5, 7}));
    CHECK(data.snapshot->facing == Direction::West);
    CHECK_EQ(data.snapshot->elapsedMs, 12345);
    CHECK_EQ(data.snapshot->berries, 2);
    CHECK_EQ(data.snapshot->collectedPerkIds.size(), static_cast<std::size_t>(2));
}

TEST_CASE(saves, garbageTextIsReportedAsCorrupt) {
    for (const char* text : {"hello world", "=\n", "version=1\nname", "\x01\x02\x03"}) {
        const io::LoadResult result = io::parseSave(text);
        CHECK(result.status == io::LoadStatus::Corrupt);
        CHECK(!result.error.empty());
    }
}

TEST_CASE(saves, truncatedFileIsReportedAsCorrupt) {
    const std::string text = io::serializeSave(sampleSave());
    for (const std::size_t length :
         {std::size_t{0}, text.size() / 4, text.size() / 2, text.size() - 40}) {
        const io::LoadResult result = io::parseSave(text.substr(0, length));
        CHECK(result.status == io::LoadStatus::Corrupt);
    }
}

TEST_CASE(saves, invalidFieldValuesAreRejected) {
    const std::string text = io::serializeSave(sampleSave());
    const std::pair<const char*, const char*> badFields[] = {{"version", "2"},
                                                             {"size", "51"},
                                                             {"size", "abc"},
                                                             {"name", ""},
                                                             {"name", "bad#name"},
                                                             {"mode", "4d"},
                                                             {"level.1", "5,100"},
                                                             {"level.1", "3"},
                                                             {"highestUnlocked", "0"},
                                                             {"selectedSkin", "nova"},
                                                             {"skins", "wisp,ghost"},
                                                             {"snapshot.playerX", "23"},
                                                             {"snapshot.berries", "9"},
                                                             {"snapshot.facing", "up"},
                                                             {"snapshot.explored", "xyz"}};
    for (const auto& field : badFields) {
        const io::LoadResult result = io::parseSave(withValue(text, field.first, field.second));
        CHECK(result.status == io::LoadStatus::Corrupt);
    }
}

TEST_CASE(saves, duplicateKeysAreRejected) {
    const std::string text = io::serializeSave(sampleSave()) + "size=20\n";
    CHECK(io::parseSave(text).status == io::LoadStatus::Corrupt);
}

TEST_CASE(saves, inconsistentProgressIsRejected) {
    // Level 5 completed although only level 3 is unlocked.
    const std::string text = withValue(io::serializeSave(sampleSave()), "level.5", "2,1000");
    CHECK(io::parseSave(text).status == io::LoadStatus::Corrupt);
}

TEST_CASE(saves, slotFilesRoundTripThroughDisk) {
    const ScratchFolder folder;
    const io::SaveSystem saves(folder.path() / "saves");
    CHECK(saves.load(1).status == io::LoadStatus::Missing);
    std::string error;
    REQUIRE(saves.save(1, sampleSave(), error));
    const io::LoadResult loaded = saves.load(1);
    CHECK(loaded.status == io::LoadStatus::Ok);
    CHECK_EQ(loaded.data.playerName, std::string("Ada 2"));
    CHECK(!std::filesystem::exists(saves.slotPath(1).string() + ".tmp"));
    CHECK(saves.erase(1));
    CHECK(saves.load(1).status == io::LoadStatus::Missing);
    CHECK(saves.load(0).status == io::LoadStatus::Corrupt);
}

TEST_CASE(saves, corruptedFileOnDiskIsDetectedWithoutCrashing) {
    const ScratchFolder folder;
    const io::SaveSystem saves(folder.path());
    std::string error;
    REQUIRE(saves.save(2, sampleSave(), error));
    {
        std::ofstream stream(saves.slotPath(2), std::ios::binary | std::ios::trunc);
        stream << "version=1\nname=Ada\nsize=";
    }
    const io::LoadResult result = saves.load(2);
    CHECK(result.status == io::LoadStatus::Corrupt);
}

TEST_CASE(saves, resumingASnapshotRestoresTheExactState) {
    Level original(4, 18, 99U, ViewMode::Classic2D);
    for (int step = 0; step < 6; ++step) {
        const auto direction = nextStepToward(original.maze(), original.player().cell(),
                                              original.maze().exit(), PassRules{true, false});
        REQUIRE(direction.has_value());
        original.tryMove(*direction);
        for (int frame = 0; frame < 30; ++frame) {
            original.update(1.0f / 60.0f);
        }
    }
    original.turnLeft();
    SaveData data;
    data.playerName = "Resume";
    data.mazeSize = 18;
    data.snapshot = original.snapshot();
    const io::LoadResult loaded = io::parseSave(io::serializeSave(data));
    REQUIRE(loaded.status == io::LoadStatus::Ok && loaded.data.snapshot.has_value());

    Level resumed(4, 18, loaded.data.snapshot->seed, ViewMode::Classic2D);
    REQUIRE(resumed.restore(*loaded.data.snapshot));
    CHECK(resumed.maze() == original.maze());
    CHECK_EQ(resumed.player().cell(), original.player().cell());
    CHECK(resumed.player().facing() == original.player().facing());
    CHECK_EQ(toMilliseconds(resumed.elapsedSeconds()), toMilliseconds(original.elapsedSeconds()));
    CHECK_EQ(resumed.timerStarted(), original.timerStarted());
}

TEST_CASE(saves, snapshotForAnotherSeedIsRefused) {
    Level level(2, 12, 5U, ViewMode::Classic2D);
    LevelSnapshot snapshot = level.snapshot();
    snapshot.seed = 6U;
    CHECK(!level.restore(snapshot));
    snapshot.seed = 5U;
    snapshot.playerCell = Cell{40, 40};
    CHECK(!level.restore(snapshot));
}

TEST_CASE(settings, settingsFileRoundTripsAndClampsValues) {
    Settings settings;
    settings.masterVolume = 35;
    settings.muted = true;
    settings.lastSlot = 2;
    const Settings parsed = io::parseSettings(io::serializeSettings(settings));
    CHECK_EQ(parsed.masterVolume, 35);
    CHECK(parsed.muted);
    CHECK_EQ(parsed.lastSlot, 2);

    const Settings clamped =
        io::parseSettings("masterVolume=300\nmusicVolume=-7\nsfxVolume=abc\nheadBob=maybe\n");
    CHECK_EQ(clamped.masterVolume, VOLUME_MAX);
    CHECK_EQ(clamped.musicVolume, VOLUME_MIN);
    CHECK_EQ(clamped.sfxVolume, DEFAULT_SFX_VOLUME);
    CHECK(clamped.headBob);
}

TEST_CASE(settings, screenShakeIsSavedAndDefaultsToOn) {
    Settings settings;
    CHECK(settings.screenShake);
    settings.screenShake = false;
    CHECK(!io::parseSettings(io::serializeSettings(settings)).screenShake);
    // settings.cfg files written before the option existed keep the shake on.
    CHECK(io::parseSettings("version=1\nmasterVolume=40\nheadBob=false\n").screenShake);
    CHECK(io::parseSettings("screenShake=sometimes\n").screenShake);
}

TEST_CASE(settings, brokenSettingsFileFallsBackToDefaults) {
    const Settings parsed = io::parseSettings("this is not a settings file");
    CHECK_EQ(parsed.masterVolume, DEFAULT_MASTER_VOLUME);
    CHECK_EQ(parsed.lastSlot, 0);
}
