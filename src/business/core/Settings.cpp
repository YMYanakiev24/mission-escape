#include "core/Settings.h"

#include <algorithm>

namespace core {

Settings clampSettings(const Settings& settings) {
    Settings clamped = settings;
    clamped.masterVolume = std::clamp(settings.masterVolume, VOLUME_MIN, VOLUME_MAX);
    clamped.musicVolume = std::clamp(settings.musicVolume, VOLUME_MIN, VOLUME_MAX);
    clamped.sfxVolume = std::clamp(settings.sfxVolume, VOLUME_MIN, VOLUME_MAX);
    clamped.lastSlot = std::clamp(settings.lastSlot, 0, SAVE_SLOT_COUNT);
    return clamped;
}

float effectiveVolume(const Settings& settings, int channelVolume) {
    if (settings.muted) {
        return 0.0f;
    }
    const Settings clamped = clampSettings(settings);
    const float master = static_cast<float>(clamped.masterVolume) / static_cast<float>(VOLUME_MAX);
    const float channel = static_cast<float>(std::clamp(channelVolume, VOLUME_MIN, VOLUME_MAX)) /
                          static_cast<float>(VOLUME_MAX);
    return master * channel;
}

} // namespace core
