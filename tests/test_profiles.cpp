#include "TestHarness.h"

#include "core/ProfileService.h"
#include "core/Repositories.h"

#include <map>
#include <string>
#include <utility>

using namespace core;

namespace {

/// In-memory stand-in for the data layer, so the business logic is tested without any files.
class FakeSaves final : public SaveRepository {
public:
    LoadResult load(int slot) const override {
        ++loads;
        const auto found = stored.find(slot);
        return found == stored.end() ? LoadResult{} : found->second;
    }

    bool save(int slot, const SaveData& data, std::string& error) const override {
        if (failWrites) {
            error = "disk full";
            return false;
        }
        stored[slot] = LoadResult{LoadStatus::Ok, data, ""};
        ++writes;
        return true;
    }

    bool erase(int slot) const override {
        stored.erase(slot);
        ++erases;
        return true;
    }

    mutable std::map<int, LoadResult> stored;
    mutable int loads = 0;
    mutable int writes = 0;
    mutable int erases = 0;
    bool failWrites = false;
};

/// In-memory settings storage.
class FakeSettings final : public SettingsRepository {
public:
    Settings load() const override { return stored; }

    bool save(const Settings& settings, std::string& /*error*/) const override {
        stored = settings;
        ++writes;
        return true;
    }

    mutable Settings stored;
    mutable int writes = 0;
};

SaveData profileNamed(const std::string& name) {
    SaveData data;
    data.playerName = name;
    return data;
}

} // namespace

TEST_CASE(profiles, loadingReportsTheStateOfEverySlot) {
    FakeSaves saves;
    FakeSettings settings;
    saves.stored[1] = LoadResult{LoadStatus::Ok, profileNamed("Ana"), ""};
    saves.stored[2] = LoadResult{LoadStatus::Corrupt, SaveData{}, "invalid record"};
    settings.stored.masterVolume = 40;
    ProfileService profiles(saves, settings, true);
    profiles.loadAll();
    CHECK_EQ(profiles.slots()[0].status, SlotStatus::Ready);
    CHECK_EQ(profiles.slots()[0].data->playerName, std::string("Ana"));
    CHECK_EQ(profiles.slots()[1].status, SlotStatus::Corrupt);
    CHECK_EQ(profiles.slots()[1].error, std::string("invalid record"));
    CHECK_EQ(profiles.slots()[2].status, SlotStatus::Empty);
    CHECK_EQ(profiles.settings().masterVolume, 40);
    CHECK(profiles.activeProfile() == nullptr);
}

TEST_CASE(profiles, creatingAProfileActivatesAndSavesIt) {
    FakeSaves saves;
    FakeSettings settings;
    ProfileService profiles(saves, settings, true);
    profiles.createProfile(2, profileNamed("Bob"));
    REQUIRE(profiles.activeProfile() != nullptr);
    CHECK_EQ(profiles.activeSlot(), 2);
    CHECK_EQ(profiles.activeProfile()->playerName, std::string("Bob"));
    CHECK(!profiles.activeProfile()->lastPlayed.empty()); // stamped when saved
    CHECK_EQ(saves.writes, 1);
    REQUIRE(saves.stored.count(2) == 1);
    CHECK_EQ(saves.stored[2].data.playerName, std::string("Bob"));
    CHECK_EQ(settings.stored.lastSlot, 2); // remembered for "Continue"
}

TEST_CASE(profiles, onlyReadySlotsCanBeActivated) {
    FakeSaves saves;
    FakeSettings settings;
    saves.stored[2] = LoadResult{LoadStatus::Corrupt, SaveData{}, "broken"};
    ProfileService profiles(saves, settings, true);
    profiles.loadAll();
    CHECK(!profiles.activateSlot(1)); // empty
    CHECK(!profiles.activateSlot(2)); // corrupt
    CHECK(!profiles.activateSlot(4)); // no such slot
    CHECK_EQ(profiles.activeSlot(), 0);
}

TEST_CASE(profiles, recordingAResultUpdatesProgressAndDropsTheSavedAttempt) {
    FakeSaves saves;
    FakeSettings settings;
    ProfileService profiles(saves, settings, true);
    SaveData data = profileNamed("Cleo");
    data.snapshot = LevelSnapshot{};
    profiles.createProfile(1, data);
    const std::optional<RecordedResult> result = profiles.recordResult(1, 3, 12000);
    REQUIRE(result.has_value());
    CHECK(result->update.levelUnlocked);
    CHECK_EQ(result->record.bestStars, 3);
    CHECK_EQ(result->record.bestTimeMs, 12000);
    CHECK_EQ(profiles.activeProfile()->progress.highestUnlockedLevel(), 2);
    CHECK(!profiles.activeProfile()->snapshot.has_value());
    CHECK(!saves.stored[1].data.snapshot.has_value()); // the saved file no longer has it
}

TEST_CASE(profiles, recordingWithoutAnActiveProfileChangesNothing) {
    FakeSaves saves;
    FakeSettings settings;
    ProfileService profiles(saves, settings, true);
    CHECK(!profiles.recordResult(1, 3, 1000).has_value());
    CHECK_EQ(saves.writes, 0);
}

TEST_CASE(profiles, saveAndExitStoresTheAttemptAndADiscardRemovesIt) {
    FakeSaves saves;
    FakeSettings settings;
    ProfileService profiles(saves, settings, true);
    profiles.createProfile(1, profileNamed("Dan"));
    LevelSnapshot snapshot;
    snapshot.level = 4;
    profiles.storeSnapshot(snapshot);
    REQUIRE(saves.stored[1].data.snapshot.has_value());
    CHECK_EQ(saves.stored[1].data.snapshot->level, 4);
    profiles.discardSavedAttempt();
    CHECK(!saves.stored[1].data.snapshot.has_value());
}

TEST_CASE(profiles, deletingTheActiveSlotForgetsIt) {
    FakeSaves saves;
    FakeSettings settings;
    ProfileService profiles(saves, settings, true);
    profiles.createProfile(3, profileNamed("Eve"));
    profiles.deleteSlot(3);
    CHECK_EQ(saves.erases, 1);
    CHECK_EQ(saves.stored.count(3), static_cast<std::size_t>(0));
    CHECK_EQ(profiles.slots()[2].status, SlotStatus::Empty);
    CHECK_EQ(profiles.activeSlot(), 0);
    CHECK_EQ(settings.stored.lastSlot, 0);
}

TEST_CASE(profiles, aFailedSaveIsReportedToTheUser) {
    FakeSaves saves;
    FakeSettings settings;
    saves.failWrites = true;
    ProfileService profiles(saves, settings, true);
    profiles.createProfile(1, profileNamed("Finn"));
    CHECK_EQ(profiles.lastError(), std::string("Could not save: disk full"));
    REQUIRE(profiles.activeProfile() != nullptr); // still playable from memory
}

TEST_CASE(profiles, disabledPersistenceNeverTouchesStorage) {
    FakeSaves saves;
    FakeSettings settings;
    ProfileService profiles(saves, settings, false);
    profiles.loadAll();
    profiles.createProfile(1, profileNamed("Gus"));
    profiles.recordResult(1, 2, 5000);
    profiles.deleteSlot(1);
    CHECK_EQ(saves.loads, 0);
    CHECK_EQ(saves.writes, 0);
    CHECK_EQ(saves.erases, 0);
    CHECK_EQ(settings.writes, 0);
}
