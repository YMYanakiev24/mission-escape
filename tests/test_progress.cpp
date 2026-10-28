#include "TestHarness.h"

#include "core/Config.h"
#include "core/Progress.h"
#include "core/Settings.h"

using namespace core;

TEST_CASE(unlocks, newProgressHasOnlyLevelOneAndWisp) {
    const Progress progress;
    CHECK_EQ(progress.highestUnlockedLevel(), 1);
    CHECK(progress.isLevelUnlocked(1));
    CHECK(!progress.isLevelUnlocked(2));
    CHECK(progress.isSkinUnlocked(SkinId::Wisp));
    CHECK(!progress.isSkinUnlocked(SkinId::Prism));
    CHECK(progress.selectedSkin() == SkinId::Wisp);
}

TEST_CASE(unlocks, completingALevelUnlocksTheNext) {
    Progress progress;
    const ProgressUpdate update = progress.applyResult(1, 2, 30000);
    CHECK(update.levelUnlocked);
    CHECK(update.newRecord);
    CHECK(progress.isLevelUnlocked(2));
    CHECK_EQ(progress.totalStars(), 2);
}

TEST_CASE(unlocks, quittingNeverUnlocksAnything) {
    Progress progress;
    const ProgressUpdate update = progress.applyResult(1, QUIT_STARS, 1000);
    CHECK(!update.levelUnlocked);
    CHECK(!progress.isLevelUnlocked(2));
    CHECK(!progress.record(1).completed);
}

TEST_CASE(unlocks, skinsUnlockOnFirstCompletionOfMilestoneLevels) {
    Progress progress;
    for (int level = 1; level <= 4; ++level) {
        CHECK(!progress.applyResult(level, 1, 1000).skinUnlocked.has_value());
    }
    const ProgressUpdate prism = progress.applyResult(5, 1, 1000);
    REQUIRE(prism.skinUnlocked.has_value());
    CHECK(*prism.skinUnlocked == SkinId::Prism);
    CHECK(progress.isSkinUnlocked(SkinId::Prism));
    // Completing level 5 again does not report the unlock a second time.
    CHECK(!progress.applyResult(5, 3, 900).skinUnlocked.has_value());
}

TEST_CASE(unlocks, skinUnlockLevelsMatchTheTable) {
    CHECK_EQ(skinUnlockLevel(SkinId::Wisp), 0);
    CHECK_EQ(skinUnlockLevel(SkinId::Prism), 5);
    CHECK_EQ(skinUnlockLevel(SkinId::Comet), 10);
    CHECK_EQ(skinUnlockLevel(SkinId::Phantom), 15);
    CHECK_EQ(skinUnlockLevel(SkinId::Nova), 20);
    CHECK(skinUnlockedByLevel(10) == SkinId::Comet);
    CHECK(!skinUnlockedByLevel(11).has_value());
}

TEST_CASE(unlocks, lockedSkinsCannotBeSelected) {
    Progress progress;
    CHECK(!progress.selectSkin(SkinId::Nova));
    CHECK(progress.selectedSkin() == SkinId::Wisp);
    progress.restoreSkinMask(0xFFU);
    CHECK(progress.selectSkin(SkinId::Nova));
    progress.restoreSkinMask(0U); // the default skin always stays unlocked
    CHECK(progress.isSkinUnlocked(SkinId::Wisp));
    CHECK(progress.selectedSkin() == SkinId::Wisp);
}

TEST_CASE(unlocks, finalLevelCompletesCampaignOnce) {
    Progress progress;
    REQUIRE(progress.restoreHighestUnlocked(LEVEL_COUNT));
    CHECK(progress.applyResult(LEVEL_COUNT, 1, 5000).campaignCompleted);
    CHECK(!progress.applyResult(LEVEL_COUNT, 3, 4000).campaignCompleted);
    CHECK_EQ(progress.highestUnlockedLevel(), LEVEL_COUNT);
}

TEST_CASE(unlocks, invalidRecordsAreRejectedOnRestore) {
    Progress progress;
    CHECK(!progress.restoreRecord(0, LevelRecord{1, 100, true}));
    CHECK(!progress.restoreRecord(3, LevelRecord{4, 100, true}));
    CHECK(!progress.restoreRecord(3, LevelRecord{2, -5, true}));
    CHECK(!progress.restoreRecord(3, LevelRecord{2, 100, false}));
    CHECK(progress.restoreRecord(3, LevelRecord{2, 100, true}));
    CHECK(!progress.restoreHighestUnlocked(21));
}

TEST_CASE(settings, outOfRangeValuesAreClamped) {
    Settings settings;
    settings.masterVolume = 250;
    settings.musicVolume = -40;
    settings.sfxVolume = 101;
    settings.lastSlot = 9;
    const Settings clamped = clampSettings(settings);
    CHECK_EQ(clamped.masterVolume, VOLUME_MAX);
    CHECK_EQ(clamped.musicVolume, VOLUME_MIN);
    CHECK_EQ(clamped.sfxVolume, VOLUME_MAX);
    CHECK_EQ(clamped.lastSlot, SAVE_SLOT_COUNT);
}

TEST_CASE(settings, effectiveVolumeCombinesMasterChannelAndMute) {
    Settings settings;
    settings.masterVolume = 50;
    CHECK_EQ(effectiveVolume(settings, 100), 0.5f);
    CHECK_EQ(effectiveVolume(settings, 50), 0.25f);
    settings.muted = true;
    CHECK_EQ(effectiveVolume(settings, 100), 0.0f);
}
