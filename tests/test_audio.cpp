#include "TestHarness.h"

#include "audio/Synth.h"

#include <algorithm>
#include <cmath>

using namespace audio;

namespace {

float peakOf(const std::vector<float>& samples) {
    float peak = 0.0f;
    for (const float sample : samples) {
        peak = std::max(peak, std::fabs(sample));
    }
    return peak;
}

} // namespace

TEST_CASE(audio, everySoundEffectIsAudibleAndNeverClips) {
    for (int index = 0; index < SOUND_COUNT; ++index) {
        const std::vector<float> samples =
            synthesizeSound(static_cast<SoundId>(index), SFX_SAMPLE_RATE);
        CHECK(!samples.empty());
        const float peak = peakOf(samples);
        CHECK(peak > 0.05f);
        CHECK(peak <= 1.0f);
        CHECK(!soundFileName(static_cast<SoundId>(index)).empty());
    }
}

TEST_CASE(audio, soundEffectsStartAndEndSilentlyToAvoidClicks) {
    for (int index = 0; index < SOUND_COUNT; ++index) {
        const std::vector<float> samples =
            synthesizeSound(static_cast<SoundId>(index), SFX_SAMPLE_RATE);
        REQUIRE(!samples.empty());
        CHECK(std::fabs(samples.front()) < 0.01f);
        CHECK(std::fabs(samples.back()) < 0.01f);
    }
}

TEST_CASE(audio, musicLoopsHaveExactLengthAndJoinSeamlessly) {
    const auto expected = static_cast<std::size_t>(MUSIC_LOOP_SECONDS * MUSIC_SAMPLE_RATE);
    for (int index = 0; index < MUSIC_COUNT; ++index) {
        const std::vector<float> loop =
            synthesizeMusic(static_cast<MusicId>(index), MUSIC_SAMPLE_RATE);
        CHECK_EQ(loop.size(), expected);
        CHECK(peakOf(loop) <= 1.0f);
        // The jump from the last sample back to the first must be as small as a normal step.
        const float seam = std::fabs(loop.front() - loop.back());
        float largestStep = 0.0f;
        for (std::size_t sample = 1; sample < loop.size(); ++sample) {
            largestStep = std::max(largestStep, std::fabs(loop[sample] - loop[sample - 1]));
        }
        CHECK(seam <= largestStep + 1e-4f);
    }
}

TEST_CASE(audio, synthesisIsDeterministic) {
    CHECK(synthesizeSound(SoundId::Phase, SFX_SAMPLE_RATE) ==
          synthesizeSound(SoundId::Phase, SFX_SAMPLE_RATE));
}
