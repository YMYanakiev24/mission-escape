#include "audio/AudioManager.h"

#include <algorithm>
#include <cstddef>

namespace audio {

namespace {

constexpr int STREAM_BUFFER_FRAMES = 4096;
constexpr unsigned int SAMPLE_BITS = 16;
constexpr unsigned int MONO = 1;
constexpr float CROSSFADE_SECONDS = 1.6f;
constexpr float PCM_SCALE = 32767.0f;
constexpr int MAX_REFILLS_PER_FRAME = 2;

short toPcm(float sample) {
    return static_cast<short>(std::clamp(sample, -1.0f, 1.0f) * PCM_SCALE);
}

/// Converts float samples into a raylib Sound (LoadSoundFromWave copies the data).
Sound soundFromSamples(const std::vector<float>& samples, int sampleRate) {
    std::vector<short> pcm(samples.size());
    std::transform(samples.begin(), samples.end(), pcm.begin(), toPcm);
    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(pcm.size());
    wave.sampleRate = static_cast<unsigned int>(sampleRate);
    wave.sampleSize = SAMPLE_BITS;
    wave.channels = MONO;
    wave.data = pcm.data();
    return LoadSoundFromWave(wave);
}

std::string overridePath(const std::string& folder, std::string_view name) {
    return folder + "/" + std::string(name) + ".ogg";
}

} // namespace

AudioManager::~AudioManager() {
    shutdown();
}

void AudioManager::initialize(const std::string& audioAssetFolder) {
    m_folder = audioAssetFolder;
    m_ready = IsAudioDeviceReady();
    if (!m_ready) {
        return;
    }
    for (int index = 0; index < SOUND_COUNT; ++index) {
        loadSound(static_cast<SoundId>(index), m_folder);
    }
    SetAudioStreamBufferSizeDefault(STREAM_BUFFER_FRAMES);
    m_stream = LoadAudioStream(MUSIC_SAMPLE_RATE, SAMPLE_BITS, MONO);
    m_mixBuffer.assign(STREAM_BUFFER_FRAMES, 0);
    PlayAudioStream(m_stream);
}

void AudioManager::loadSound(SoundId id, const std::string& folder) {
    Sound& sound = m_sounds[static_cast<std::size_t>(id)];
    const std::string path = overridePath(folder, soundFileName(id));
    if (FileExists(path.c_str())) {
        sound = LoadSound(path.c_str());
        if (IsSoundValid(sound)) {
            return;
        }
    }
    sound = soundFromSamples(synthesizeSound(id, SFX_SAMPLE_RATE), SFX_SAMPLE_RATE);
}

void AudioManager::shutdown() {
    if (!m_ready) {
        return;
    }
    for (Sound& sound : m_sounds) {
        if (IsSoundValid(sound)) {
            UnloadSound(sound);
        }
        sound = Sound{};
    }
    for (MusicTrack& track : m_tracks) {
        if (track.fromFile) {
            UnloadMusicStream(track.file);
        }
        track = MusicTrack{};
    }
    if (IsAudioStreamValid(m_stream)) {
        UnloadAudioStream(m_stream);
    }
    m_stream = AudioStream{};
    m_ready = false;
}

void AudioManager::applySettings(const core::Settings& settings) {
    m_musicGain = core::effectiveVolume(settings, settings.musicVolume);
    m_sfxGain = core::effectiveVolume(settings, settings.sfxVolume);
}

void AudioManager::play(SoundId id, float volume, float pitch) {
    if (!m_ready || m_sfxGain <= 0.0f) {
        return;
    }
    const Sound& sound = m_sounds[static_cast<std::size_t>(id)];
    SetSoundVolume(sound, std::clamp(m_sfxGain * volume, 0.0f, 1.0f));
    SetSoundPitch(sound, pitch);
    PlaySound(sound);
}

void AudioManager::ensureTrack(MusicId id) {
    MusicTrack& track = m_tracks[static_cast<std::size_t>(id)];
    if (track.loaded) {
        return;
    }
    const std::string path = overridePath(m_folder, musicFileName(id));
    if (FileExists(path.c_str())) {
        track.file = LoadMusicStream(path.c_str());
        track.fromFile = IsMusicValid(track.file);
        track.file.looping = true;
    }
    if (!track.fromFile) {
        track.samples = synthesizeMusic(id, MUSIC_SAMPLE_RATE);
    }
    track.loaded = true;
}

void AudioManager::playMusic(MusicId id) {
    const int index = static_cast<int>(id);
    if (!m_ready || index == m_current) {
        return;
    }
    ensureTrack(id);
    if (m_previous >= 0 && m_tracks[static_cast<std::size_t>(m_previous)].fromFile) {
        StopMusicStream(m_tracks[static_cast<std::size_t>(m_previous)].file);
    }
    m_previous = m_current;
    m_current = index;
    m_crossfade = 0.0f;
    MusicTrack& track = m_tracks[static_cast<std::size_t>(index)];
    if (track.fromFile) {
        SeekMusicStream(track.file, 0.0f);
        PlayMusicStream(track.file);
    }
}

float AudioManager::nextMusicSample(MusicTrack& track) {
    if (track.fromFile || track.samples.empty()) {
        return 0.0f;
    }
    const float sample = track.samples[track.position];
    track.position = (track.position + 1) % track.samples.size();
    return sample;
}

void AudioManager::fillStream() {
    for (short& frame : m_mixBuffer) {
        float mixed = 0.0f;
        if (m_current >= 0) {
            mixed += m_crossfade * nextMusicSample(m_tracks[static_cast<std::size_t>(m_current)]);
        }
        if (m_previous >= 0) {
            mixed += (1.0f - m_crossfade) *
                     nextMusicSample(m_tracks[static_cast<std::size_t>(m_previous)]);
        }
        frame = toPcm(mixed * m_musicGain);
    }
    UpdateAudioStream(m_stream, m_mixBuffer.data(), static_cast<int>(m_mixBuffer.size()));
}

void AudioManager::updateFileMusic(float /*deltaSeconds*/) {
    for (const int index : {m_current, m_previous}) {
        if (index < 0 || !m_tracks[static_cast<std::size_t>(index)].fromFile) {
            continue;
        }
        Music& file = m_tracks[static_cast<std::size_t>(index)].file;
        const float fade = index == m_current ? m_crossfade : 1.0f - m_crossfade;
        SetMusicVolume(file, m_musicGain * fade);
        UpdateMusicStream(file);
    }
}

void AudioManager::update(float deltaSeconds) {
    if (!m_ready) {
        return;
    }
    m_crossfade = std::min(1.0f, m_crossfade + deltaSeconds / CROSSFADE_SECONDS);
    if (m_crossfade >= 1.0f && m_previous >= 0) {
        if (m_tracks[static_cast<std::size_t>(m_previous)].fromFile) {
            StopMusicStream(m_tracks[static_cast<std::size_t>(m_previous)].file);
        }
        m_previous = -1;
    }
    updateFileMusic(deltaSeconds);
    for (int refill = 0; refill < MAX_REFILLS_PER_FRAME && IsAudioStreamProcessed(m_stream);
         ++refill) {
        fillStream();
    }
}

} // namespace audio
