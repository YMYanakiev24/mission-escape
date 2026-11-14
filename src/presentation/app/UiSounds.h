#pragma once

#include "audio/AudioManager.h"
#include "ui/Widgets.h"

/// @file UiSounds.h
/// Connects widget focus/activation notifications to the menu sound effects.

namespace app {

/// Plays the hover and click sounds for every FocusGroup that uses it.
class UiSounds : public ui::UiFeedback {
public:
    /// Plays through @p audio, which must outlive this object.
    explicit UiSounds(audio::AudioManager& audio) : m_audio(audio) {}

    /// Plays the hover tick.
    void focusMoved() override { m_audio.play(audio::SoundId::MenuHover); }
    /// Plays the click.
    void activated() override { m_audio.play(audio::SoundId::MenuClick); }

private:
    audio::AudioManager& m_audio;
};

} // namespace app
