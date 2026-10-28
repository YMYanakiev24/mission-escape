#pragma once

#include "audio/Synth.h"
#include "core/Settings.h"

#include <raylib.h>

#include <array>
#include <string>
#include <vector>

/// @file AudioManager.h
/// Plays the synthesised sound effects and streams the ambient music loops.

namespace audio {

/// Owns every sound and the music stream. All calls are safe no-ops when no audio device is
/// available, so the game also runs on machines without sound.
///
/// Sound effects are synthesised at start-up (or loaded from `assets/audio/<name>.ogg` when
/// such a file exists). Music loops are synthesised lazily the first time they are needed and
/// streamed through one raylib AudioStream, crossfading when the track changes.
class AudioManager {
public:
    AudioManager() = default;
    /// Releases all sounds and the music stream.
    ~AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    /// Loads or synthesises all sounds; call after InitAudioDevice().
    void initialize(const std::string& audioAssetFolder);

    /// Releases sounds and the stream (call before CloseAudioDevice()).
    void shutdown();

    /// True when sound output works.
    bool isReady() const noexcept { return m_ready; }

    /// Applies master/music/sfx volumes and mute.
    void applySettings(const core::Settings& settings);

    /// Plays a sound effect; @p volume scales it and @p pitch shifts it (1 = original).
    void play(SoundId id, float volume = 1.0f, float pitch = 1.0f);

    /// Switches the background loop with a crossfade (no-op if it is already playing).
    void playMusic(MusicId id);

    /// Feeds the music stream; call once per frame.
    void update(float deltaSeconds);

private:
    /// A music loop: either synthesised samples or a streamed file.
    struct MusicTrack {
        std::vector<float> samples;
        std::size_t position = 0;
        Music file{};
        bool fromFile = false;
        bool loaded = false;
    };

    void loadSound(SoundId id, const std::string& folder);
    void ensureTrack(MusicId id);
    float nextMusicSample(MusicTrack& track);
    void fillStream();
    void updateFileMusic(float deltaSeconds);

    bool m_ready = false;
    std::string m_folder;
    std::array<Sound, SOUND_COUNT> m_sounds{};
    std::array<MusicTrack, MUSIC_COUNT> m_tracks{};
    AudioStream m_stream{};
    int m_current = -1;  ///< Track fading in / playing (-1 = none).
    int m_previous = -1; ///< Track fading out (-1 = none).
    float m_crossfade = 1.0f;
    float m_musicGain = 0.0f;
    float m_sfxGain = 0.0f;
    std::vector<short> m_mixBuffer;
};

} // namespace audio
