#include "TestHarness.h"

#include "core/Config.h"
#include "core/Scoring.h"

using namespace core;

TEST_CASE(stars, parTimeFollowsFormula) {
    CHECK_EQ(parTime(100, Tier::Awakening, ViewMode::Classic2D), 100 * SECONDS_PER_CELL);
    CHECK_EQ(parTime(100, Tier::Fog, ViewMode::Classic2D),
             100 * SECONDS_PER_CELL * TIME_MULTIPLIER_FOG);
    CHECK_EQ(parTime(100, Tier::Awakening, ViewMode::FirstPerson3D),
             100 * SECONDS_PER_CELL * TIME_MULTIPLIER_3D);
    CHECK_EQ(parTime(-5, Tier::Hunted, ViewMode::Classic2D), 0.0f);
}

TEST_CASE(stars, exactlyOnThreeStarThresholdEarnsThreeStars) {
    const float par = 10.0f;
    CHECK_EQ(starsForTime(par * THREE_STAR_FACTOR, par), 3);
    CHECK_EQ(starsForTime(0.0f, par), 3);
}

TEST_CASE(stars, justAboveThreeStarThresholdEarnsTwoStars) {
    const float par = 10.0f;
    CHECK_EQ(starsForTime(par * THREE_STAR_FACTOR + 0.01f, par), 2);
}

TEST_CASE(stars, exactlyOnTwoStarThresholdEarnsTwoStars) {
    const float par = 10.0f;
    CHECK_EQ(starsForTime(par * TWO_STAR_FACTOR, par), 2);
}

TEST_CASE(stars, slowerThanTwoStarThresholdEarnsOneStar) {
    const float par = 10.0f;
    CHECK_EQ(starsForTime(par * TWO_STAR_FACTOR + 0.01f, par), 1);
    CHECK_EQ(starsForTime(100000.0f, par), 1);
}

TEST_CASE(stars, starTargetCountsDownToNextThreshold) {
    const float par = 10.0f;
    const StarTarget early = starTargetAt(5.0f, par);
    CHECK_EQ(early.stars, 3);
    CHECK_EQ(early.secondsLeft, 15.0f);
    const StarTarget middle = starTargetAt(25.0f, par);
    CHECK_EQ(middle.stars, 2);
    CHECK_EQ(middle.secondsLeft, 10.0f);
    CHECK_EQ(starTargetAt(40.0f, par).stars, 1);
}

TEST_CASE(stars, quittingGivesZeroStarsAndKeepsPreviousBest) {
    LevelRecord record;
    CHECK(updateRecord(record, 2, 50000));
    CHECK(!updateRecord(record, QUIT_STARS, 1000));
    CHECK_EQ(record.bestStars, 2);
    CHECK_EQ(record.bestTimeMs, 50000);
    CHECK(record.completed);
}

TEST_CASE(stars, recordKeepsBestStarsAndBestTimeIndependently) {
    LevelRecord record;
    updateRecord(record, 3, 40000);
    CHECK(!updateRecord(record, 2, 45000)); // worse in both respects
    CHECK(updateRecord(record, 1, 30000));  // faster, fewer stars
    CHECK_EQ(record.bestStars, 3);
    CHECK_EQ(record.bestTimeMs, 30000);
}

TEST_CASE(stars, millisecondConversionClampsNegativeAndHugeValues) {
    CHECK_EQ(toMilliseconds(-3.0f), 0);
    CHECK_EQ(toMilliseconds(1.2345f), 1235);
    CHECK_EQ(toMilliseconds(1.0e9f), MAX_RECORD_MILLISECONDS);
}

TEST_CASE(stars, durationFormatting) {
    CHECK_EQ(formatDuration(0.0f), std::string("0:00.0"));
    CHECK_EQ(formatDuration(65.37f), std::string("1:05.3"));
    CHECK_EQ(formatDuration(-1.0f), std::string("0:00.0"));
}
