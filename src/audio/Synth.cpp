#include "audio/Synth.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace audio {

namespace {

constexpr double TWO_PI = 6.283185307179586;
constexpr double PI_VALUE = 3.141592653589793;
constexpr float CLICK_GUARD_SECONDS = 0.004f;
constexpr int SQUARE_HARMONICS = 5;
constexpr int NOTES_PER_CHORD = 4;
constexpr int CHORDS_PER_LOOP = 4;
constexpr float CHORD_SECONDS = MUSIC_LOOP_SECONDS / CHORDS_PER_LOOP;
constexpr float CHORD_FADE_SECONDS = 1.5f;
constexpr float CONCERT_A = 440.0f;
constexpr int CONCERT_A_MIDI = 69;
constexpr float SEMITONES_PER_OCTAVE = 12.0f;

/// A fixed-length mono sample buffer that sounds are mixed into.
class Signal {
public:
    Signal(float seconds, int sampleRate)
        : m_sampleRate(sampleRate),
          m_samples(static_cast<std::size_t>(seconds * static_cast<float>(sampleRate)), 0.0f) {}

    int sampleRate() const noexcept { return m_sampleRate; }
    std::size_t size() const noexcept { return m_samples.size(); }
    double timeAt(std::size_t index) const {
        return static_cast<double>(index) / static_cast<double>(m_sampleRate);
    }
    std::size_t indexAt(float seconds) const {
        return std::min(size(), static_cast<std::size_t>(std::max(0.0f, seconds) *
                                                         static_cast<float>(m_sampleRate)));
    }
    float& operator[](std::size_t index) { return m_samples[index]; }

    /// Normalises the loudest sample to @p peak and optionally fades the edges against clicks.
    std::vector<float> finish(float peak, bool fadeEdges) {
        float loudest = 0.0f;
        for (const float sample : m_samples) {
            loudest = std::max(loudest, std::fabs(sample));
        }
        const float gain = loudest > 0.0f ? peak / loudest : 0.0f;
        const std::size_t guard = indexAt(CLICK_GUARD_SECONDS);
        for (std::size_t index = 0; index < size(); ++index) {
            float edge = 1.0f;
            if (fadeEdges && guard > 0) {
                edge =
                    std::min({1.0f, static_cast<float>(index) / static_cast<float>(guard),
                              static_cast<float>(size() - 1 - index) / static_cast<float>(guard)});
            }
            m_samples[index] *= gain * edge;
        }
        return m_samples;
    }

private:
    int m_sampleRate;
    std::vector<float> m_samples;
};

/// Deterministic white noise (a tiny LCG keeps every build sounding the same).
class Noise {
public:
    explicit Noise(std::uint32_t seed) : m_state(seed) {}
    float next() {
        m_state = m_state * 1664525U + 1013904223U;
        return static_cast<float>(m_state >> 8U) / 8388608.0f - 1.0f;
    }

private:
    std::uint32_t m_state;
};

/// One-pole low-pass filter for softening noise and square waves.
class LowPass {
public:
    float process(float input, float cutoffHz, int sampleRate) {
        const float alpha = 1.0f - std::exp(static_cast<float>(-TWO_PI) * cutoffHz /
                                            static_cast<float>(sampleRate));
        m_value += alpha * (input - m_value);
        return m_value;
    }

private:
    float m_value = 0.0f;
};

float noteHz(int midi) {
    return CONCERT_A *
           std::pow(2.0f, static_cast<float>(midi - CONCERT_A_MIDI) / SEMITONES_PER_OCTAVE);
}

/// Rounds a frequency so that it completes a whole number of cycles per music loop.
double loopSafe(double frequency) {
    return std::round(frequency * MUSIC_LOOP_SECONDS) / MUSIC_LOOP_SECONDS;
}

/// Attack then exponential decay.
float pluck(double age, float attack, float decayRate) {
    if (age < attack) {
        return static_cast<float>(age / attack);
    }
    return std::exp(-decayRate * static_cast<float>(age - attack));
}

/// Adds a sine tone with an optional second harmonic.
void addTone(Signal& signal, float start, float duration, float frequency, float amplitude,
             float decayRate, float secondHarmonic = 0.0f) {
    const std::size_t first = signal.indexAt(start);
    const std::size_t last = signal.indexAt(start + duration);
    for (std::size_t index = first; index < last; ++index) {
        const double age = signal.timeAt(index) - start;
        const double phase = TWO_PI * frequency * age;
        const float wave =
            static_cast<float>(std::sin(phase) + secondHarmonic * std::sin(2.0 * phase));
        signal[index] += amplitude * pluck(age, 0.004f, decayRate) * wave;
    }
}

/// Adds a sine whose pitch glides from @p from to @p to.
void addSweep(Signal& signal, float start, float duration, float from, float to, float amplitude,
              float decayRate) {
    const std::size_t first = signal.indexAt(start);
    const std::size_t last = signal.indexAt(start + duration);
    double phase = 0.0;
    for (std::size_t index = first; index < last; ++index) {
        const double age = signal.timeAt(index) - start;
        const double progress = age / duration;
        phase += TWO_PI * (from + (to - from) * progress) / signal.sampleRate();
        signal[index] +=
            amplitude * pluck(age, 0.003f, decayRate) * static_cast<float>(std::sin(phase));
    }
}

/// Adds low-passed noise.
void addNoise(Signal& signal, float start, float duration, float cutoffHz, float amplitude,
              float decayRate, std::uint32_t seed) {
    Noise noise(seed);
    LowPass filter;
    const std::size_t first = signal.indexAt(start);
    const std::size_t last = signal.indexAt(start + duration);
    for (std::size_t index = first; index < last; ++index) {
        const double age = signal.timeAt(index) - start;
        const float sample = filter.process(noise.next(), cutoffHz, signal.sampleRate());
        signal[index] += amplitude * pluck(age, 0.002f, decayRate) * sample;
    }
}

/// Adds a soft (band-limited) square wave gliding in pitch, used for the harsh "caught" sting.
void addSquareSweep(Signal& signal, float duration, float from, float to, float amplitude,
                    float decayRate) {
    double phase = 0.0;
    const std::size_t last = signal.indexAt(duration);
    for (std::size_t index = 0; index < last; ++index) {
        const double age = signal.timeAt(index);
        phase += TWO_PI * (from + (to - from) * (age / duration)) / signal.sampleRate();
        double wave = 0.0;
        for (int harmonic = 1; harmonic <= 2 * SQUARE_HARMONICS - 1; harmonic += 2) {
            wave += std::sin(phase * harmonic) / harmonic;
        }
        signal[index] += amplitude * pluck(age, 0.01f, decayRate) * static_cast<float>(wave);
    }
}

// ----------------------------------------------------------------------------------------
// Sound effects
// ----------------------------------------------------------------------------------------

std::vector<float> stepSound(int rate) {
    Signal signal(0.07f, rate);
    addNoise(signal, 0.0f, 0.05f, 1400.0f, 0.5f, 60.0f, 11U);
    addTone(signal, 0.0f, 0.06f, 150.0f, 0.5f, 45.0f);
    return signal.finish(0.30f, true);
}

std::vector<float> bumpSound(int rate) {
    Signal signal(0.14f, rate);
    addSweep(signal, 0.0f, 0.13f, 130.0f, 55.0f, 0.9f, 22.0f);
    addNoise(signal, 0.0f, 0.06f, 500.0f, 0.4f, 50.0f, 23U);
    return signal.finish(0.55f, true);
}

std::vector<float> pickupSound(int rate) {
    Signal signal(0.55f, rate);
    addTone(signal, 0.0f, 0.3f, noteHz(81), 0.6f, 9.0f, 0.3f);
    addTone(signal, 0.09f, 0.42f, noteHz(88), 0.6f, 7.0f, 0.3f);
    addTone(signal, 0.18f, 0.35f, noteHz(93), 0.3f, 9.0f);
    return signal.finish(0.55f, true);
}

std::vector<float> phaseSound(int rate) {
    const float duration = 0.75f;
    Signal signal(duration, rate);
    Noise noise(37U);
    LowPass filter;
    for (std::size_t index = 0; index < signal.size(); ++index) {
        const double t = signal.timeAt(index) / duration;
        const float swell = static_cast<float>(std::sin(PI_VALUE * t));
        const float cutoff = 300.0f + 2700.0f * swell;
        signal[index] += swell * swell * filter.process(noise.next(), cutoff, rate);
    }
    addSweep(signal, 0.0f, 0.7f, 180.0f, 720.0f, 0.25f, 2.0f);
    return signal.finish(0.5f, true);
}

std::vector<float> secretDoorSound(int rate) {
    Signal signal(0.75f, rate);
    addNoise(signal, 0.0f, 0.6f, 180.0f, 0.7f, 5.0f, 41U);
    for (const int midi : {72, 79, 84}) {
        addTone(signal, 0.05f, 0.65f, noteHz(midi), 0.25f, 4.0f, 0.2f);
    }
    return signal.finish(0.5f, true);
}

std::vector<float> starDingSound(int rate) {
    Signal signal(0.55f, rate);
    addTone(signal, 0.0f, 0.55f, noteHz(91), 0.7f, 7.0f, 0.25f);
    return signal.finish(0.5f, true);
}

std::vector<float> levelCompleteSound(int rate) {
    Signal signal(1.2f, rate);
    const std::array<int, 4> notes = {72, 76, 79, 84};
    for (std::size_t index = 0; index < notes.size(); ++index) {
        addTone(signal, 0.11f * static_cast<float>(index), 0.75f, noteHz(notes[index]), 0.5f, 4.5f,
                0.3f);
    }
    return signal.finish(0.6f, true);
}

std::vector<float> caughtSound(int rate) {
    Signal signal(0.85f, rate);
    for (const int midi : {57, 58, 64}) {
        addSquareSweep(signal, 0.8f, noteHz(midi), noteHz(midi) * 0.7f, 0.3f, 3.5f);
    }
    addNoise(signal, 0.0f, 0.3f, 900.0f, 0.4f, 10.0f, 53U);
    return signal.finish(0.65f, true);
}

std::vector<float> heartbeatSound(int rate) {
    Signal signal(0.55f, rate);
    addSweep(signal, 0.0f, 0.16f, 70.0f, 45.0f, 1.0f, 14.0f);
    addSweep(signal, 0.22f, 0.16f, 62.0f, 40.0f, 0.75f, 16.0f);
    return signal.finish(0.85f, true);
}

std::vector<float> menuHoverSound(int rate) {
    Signal signal(0.04f, rate);
    addTone(signal, 0.0f, 0.04f, 1250.0f, 0.6f, 90.0f);
    return signal.finish(0.2f, true);
}

std::vector<float> menuClickSound(int rate) {
    Signal signal(0.1f, rate);
    addSweep(signal, 0.0f, 0.09f, 660.0f, 990.0f, 0.7f, 28.0f);
    addNoise(signal, 0.0f, 0.006f, 4000.0f, 0.3f, 300.0f, 67U);
    return signal.finish(0.35f, true);
}

// ----------------------------------------------------------------------------------------
// Music
// ----------------------------------------------------------------------------------------

/// The four chords (MIDI notes, 0 = silent) and texture of one ambient loop.
struct PadVoicing {
    int chords[CHORDS_PER_LOOP][NOTES_PER_CHORD];
    int droneNote = 0;      ///< Sustained bass note (0 = none).
    float tremoloHz = 0.0f; ///< Amplitude pulsing for tension (0 = none).
    bool sparkles = false;  ///< High bell pings.
};

PadVoicing voicingFor(MusicId id) {
    switch (id) {
        case MusicId::Menu:
            return PadVoicing{
                {{57, 60, 64, 71}, {53, 57, 60, 64}, {55, 59, 62, 67}, {52, 55, 60, 64}},
                45,
                0.0f,
                true};
        case MusicId::Awakening:
            return PadVoicing{
                {{60, 64, 67, 71}, {57, 60, 64, 67}, {53, 57, 60, 64}, {55, 59, 62, 67}},
                48,
                0.0f,
                true};
        case MusicId::Labyrinth:
            return PadVoicing{
                {{50, 53, 57, 64}, {46, 50, 53, 57}, {43, 46, 50, 53}, {45, 50, 52, 57}},
                38,
                0.0f,
                false};
        case MusicId::Fog:
            return PadVoicing{{{40, 47, 55, 0}, {40, 47, 54, 0}, {43, 47, 55, 0}, {38, 45, 52, 0}},
                              28,
                              0.0f,
                              true};
        case MusicId::Hunted:
            return PadVoicing{{{48, 51, 55, 0}, {44, 48, 51, 0}, {41, 44, 48, 0}, {43, 47, 50, 0}},
                              36,
                              2.0f,
                              false};
        case MusicId::Count:
            break;
    }
    return PadVoicing{};
}

/// Smooth window of one chord inside the loop (wraps around the loop end).
float chordWeight(double time, int chord) {
    const double local = std::fmod(time - chord * CHORD_SECONDS + MUSIC_LOOP_SECONDS,
                                   static_cast<double>(MUSIC_LOOP_SECONDS));
    const auto smooth = [](double x) {
        return static_cast<float>(x * x * (3.0 - 2.0 * x));
    };
    if (local < CHORD_FADE_SECONDS) {
        return smooth(local / CHORD_FADE_SECONDS);
    }
    if (local < CHORD_SECONDS) {
        return 1.0f;
    }
    if (local < CHORD_SECONDS + CHORD_FADE_SECONDS) {
        return 1.0f - smooth((local - CHORD_SECONDS) / CHORD_FADE_SECONDS);
    }
    return 0.0f;
}

/// Adds one pad note: three slightly detuned partials plus a quiet octave for shimmer.
void addPadNote(Signal& signal, int chord, int midi) {
    const double base = noteHz(midi);
    const std::array<double, 4> partials = {loopSafe(base), loopSafe(base * 1.004),
                                            loopSafe(base * 0.996), loopSafe(base * 2.0)};
    const std::array<float, 4> levels = {0.33f, 0.28f, 0.28f, 0.12f};
    for (std::size_t index = 0; index < signal.size(); ++index) {
        const double time = signal.timeAt(index);
        const float weight = chordWeight(time, chord);
        if (weight <= 0.0f) {
            continue;
        }
        float sample = 0.0f;
        for (std::size_t partial = 0; partial < partials.size(); ++partial) {
            sample +=
                levels[partial] * static_cast<float>(std::sin(TWO_PI * partials[partial] * time));
        }
        signal[index] += weight * sample;
    }
}

void addDrone(Signal& signal, int midi) {
    const double frequency = loopSafe(noteHz(midi));
    const double lfo = 1.0 / (MUSIC_LOOP_SECONDS / 2.0); // two swells per loop
    for (std::size_t index = 0; index < signal.size(); ++index) {
        const double time = signal.timeAt(index);
        const float swell = 0.6f + 0.4f * static_cast<float>(std::sin(TWO_PI * lfo * time));
        signal[index] += 0.7f * swell * static_cast<float>(std::sin(TWO_PI * frequency * time));
    }
}

void addSparkles(Signal& signal, const PadVoicing& voicing) {
    const std::array<float, 4> times = {1.0f, 5.3f, 8.7f, 12.1f};
    for (std::size_t index = 0; index < times.size(); ++index) {
        const int note = voicing.chords[index][index % 3] + 24;
        addTone(signal, times[index], 3.4f, static_cast<float>(loopSafe(noteHz(note))), 0.35f,
                1.6f);
    }
}

void applyTremolo(Signal& signal, float frequency) {
    for (std::size_t index = 0; index < signal.size(); ++index) {
        const double time = signal.timeAt(index);
        signal[index] *= 0.75f + 0.25f * static_cast<float>(std::sin(TWO_PI * frequency * time));
    }
}

} // namespace

std::string_view soundFileName(SoundId id) {
    switch (id) {
        case SoundId::Step:
            return "step";
        case SoundId::Bump:
            return "bump";
        case SoundId::Pickup:
            return "pickup";
        case SoundId::Phase:
            return "phase";
        case SoundId::SecretDoor:
            return "secret_door";
        case SoundId::StarDing:
            return "star";
        case SoundId::LevelComplete:
            return "level_complete";
        case SoundId::Caught:
            return "caught";
        case SoundId::Heartbeat:
            return "heartbeat";
        case SoundId::MenuHover:
            return "menu_hover";
        case SoundId::MenuClick:
            return "menu_click";
        case SoundId::Count:
            break;
    }
    return "";
}

std::string_view musicFileName(MusicId id) {
    switch (id) {
        case MusicId::Menu:
            return "music_menu";
        case MusicId::Awakening:
            return "music_tier1";
        case MusicId::Labyrinth:
            return "music_tier2";
        case MusicId::Fog:
            return "music_tier3";
        case MusicId::Hunted:
            return "music_tier4";
        case MusicId::Count:
            break;
    }
    return "";
}

std::vector<float> synthesizeSound(SoundId id, int sampleRate) {
    switch (id) {
        case SoundId::Step:
            return stepSound(sampleRate);
        case SoundId::Bump:
            return bumpSound(sampleRate);
        case SoundId::Pickup:
            return pickupSound(sampleRate);
        case SoundId::Phase:
            return phaseSound(sampleRate);
        case SoundId::SecretDoor:
            return secretDoorSound(sampleRate);
        case SoundId::StarDing:
            return starDingSound(sampleRate);
        case SoundId::LevelComplete:
            return levelCompleteSound(sampleRate);
        case SoundId::Caught:
            return caughtSound(sampleRate);
        case SoundId::Heartbeat:
            return heartbeatSound(sampleRate);
        case SoundId::MenuHover:
            return menuHoverSound(sampleRate);
        case SoundId::MenuClick:
            return menuClickSound(sampleRate);
        case SoundId::Count:
            break;
    }
    return {};
}

std::vector<float> synthesizeMusic(MusicId id, int sampleRate) {
    const PadVoicing voicing = voicingFor(id);
    Signal signal(MUSIC_LOOP_SECONDS, sampleRate);
    for (int chord = 0; chord < CHORDS_PER_LOOP; ++chord) {
        for (const int midi : voicing.chords[chord]) {
            if (midi > 0) {
                addPadNote(signal, chord, midi);
            }
        }
    }
    if (voicing.droneNote > 0) {
        addDrone(signal, voicing.droneNote);
    }
    if (voicing.sparkles) {
        addSparkles(signal, voicing);
    }
    if (voicing.tremoloHz > 0.0f) {
        applyTremolo(signal, voicing.tremoloHz);
    }
    return signal.finish(0.7f, false); // loops must not be faded at the seam
}

} // namespace audio
