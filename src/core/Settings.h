#pragma once

#include "core/Config.h"

/// @file Settings.h
/// User preferences that are independent of any save slot (stored in settings.cfg).

namespace core {

/// Audio, display and convenience preferences.
struct Settings {
    int masterVolume = DEFAULT_MASTER_VOLUME; ///< 0..100
    int musicVolume = DEFAULT_MUSIC_VOLUME;   ///< 0..100
    int sfxVolume = DEFAULT_SFX_VOLUME;       ///< 0..100
    bool muted = false;
    bool fullscreen = false;
    bool showFps = false;
    bool headBob = true;     ///< Subtle camera bob while walking in 3D.
    bool screenShake = true; ///< Shake the view when the Hunter catches the player.
    int lastSlot = 0;        ///< Slot used by "Continue" (0 = none).
};

/// Returns @p settings with every value forced into its valid range.
Settings clampSettings(const Settings& settings);

/// Effective 0..1 gain of a channel (master * channel, or 0 when muted).
float effectiveVolume(const Settings& settings, int channelVolume);

} // namespace core
