#pragma once

#include <string_view>
#include <vector>

/// @file Synth.h
/// Procedural sound design: every sound effect and music loop is synthesised from oscillators,
/// noise and envelopes at start-up, so the game ships without any audio files.
/// These functions are pure (no raylib, no I/O) and produce mono float samples in [-1, 1].

namespace audio {

/// Sample rate of the sound effects.
inline constexpr int SFX_SAMPLE_RATE = 44100;
/// Sample rate of the music loops (pads need no high frequencies).
inline constexpr int MUSIC_SAMPLE_RATE = 22050;
/// Length of one seamless music loop.
inline constexpr float MUSIC_LOOP_SECONDS = 16.0f;

/// Every sound effect of the game.
enum class SoundId {
    Step,
    Bump,
    Pickup,
    Phase,
    SecretDoor,
    StarDing,
    LevelComplete,
    Caught,
    Heartbeat,
    MenuHover,
    MenuClick,
    Count
};

/// One ambient music loop per tier plus the menu.
enum class MusicId { Menu, Awakening, Labyrinth, Fog, Hunted, Count };

inline constexpr int SOUND_COUNT = static_cast<int>(SoundId::Count);
inline constexpr int MUSIC_COUNT = static_cast<int>(MusicId::Count);

/// File name (without extension) that may override a synthesised sound ("step", ...).
std::string_view soundFileName(SoundId id);

/// File name (without extension) that may override a synthesised loop ("music_menu", ...).
std::string_view musicFileName(MusicId id);

/// Synthesises one sound effect.
std::vector<float> synthesizeSound(SoundId id, int sampleRate);

/// Synthesises one seamless music loop of MUSIC_LOOP_SECONDS. All oscillator frequencies
/// are quantised to whole cycles per loop, so the end joins the start without a click.
std::vector<float> synthesizeMusic(MusicId id, int sampleRate);

} // namespace audio
