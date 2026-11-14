#include "io/SettingsStore.h"

#include "core/Config.h"
#include "io/KeyValueDocument.h"

#include <limits>
#include <utility>

namespace io {

namespace {

constexpr std::uintmax_t MAX_SETTINGS_BYTES = 4096;
constexpr long long ANY_MIN = std::numeric_limits<int>::min();
constexpr long long ANY_MAX = std::numeric_limits<int>::max();

/// Reads an integer; any number is accepted here because clampSettings() fixes the range.
int readInt(const KeyValueDocument& document, const char* key, int fallback) {
    return static_cast<int>(document.getInt(key, ANY_MIN, ANY_MAX).value_or(fallback));
}

bool readBool(const KeyValueDocument& document, const char* key, bool fallback) {
    return document.getBool(key).value_or(fallback);
}

} // namespace

std::string serializeSettings(const core::Settings& settings) {
    const core::Settings clamped = core::clampSettings(settings);
    KeyValueDocument document;
    document.setInt("version", core::SETTINGS_FORMAT_VERSION);
    document.setInt("masterVolume", clamped.masterVolume);
    document.setInt("musicVolume", clamped.musicVolume);
    document.setInt("sfxVolume", clamped.sfxVolume);
    document.setBool("muted", clamped.muted);
    document.setBool("fullscreen", clamped.fullscreen);
    document.setBool("showFps", clamped.showFps);
    document.setBool("headBob", clamped.headBob);
    document.setBool("screenShake", clamped.screenShake);
    document.setInt("lastSlot", clamped.lastSlot);
    return document.serialize();
}

core::Settings parseSettings(std::string_view text) {
    const core::Settings defaults;
    std::string error;
    const std::optional<KeyValueDocument> document = KeyValueDocument::parse(text, error);
    if (!document) {
        return defaults;
    }
    core::Settings settings;
    settings.masterVolume = readInt(*document, "masterVolume", defaults.masterVolume);
    settings.musicVolume = readInt(*document, "musicVolume", defaults.musicVolume);
    settings.sfxVolume = readInt(*document, "sfxVolume", defaults.sfxVolume);
    settings.muted = readBool(*document, "muted", defaults.muted);
    settings.fullscreen = readBool(*document, "fullscreen", defaults.fullscreen);
    settings.showFps = readBool(*document, "showFps", defaults.showFps);
    settings.headBob = readBool(*document, "headBob", defaults.headBob);
    settings.screenShake = readBool(*document, "screenShake", defaults.screenShake);
    settings.lastSlot = readInt(*document, "lastSlot", defaults.lastSlot);
    return core::clampSettings(settings);
}

SettingsStore::SettingsStore(std::filesystem::path file) : m_file(std::move(file)) {}

core::Settings SettingsStore::load() const {
    std::string error;
    const std::optional<std::string> text = readTextFile(m_file, MAX_SETTINGS_BYTES, error);
    return text ? parseSettings(*text) : core::Settings{};
}

bool SettingsStore::save(const core::Settings& settings, std::string& error) const {
    return writeTextFileAtomically(m_file, serializeSettings(settings), error);
}

} // namespace io
