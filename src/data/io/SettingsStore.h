#pragma once

#include "core/Settings.h"

#include <filesystem>
#include <string>
#include <string_view>

/// @file SettingsStore.h
/// Persistence of user preferences in `settings.cfg`.

namespace io {

/// Serialises settings as key=value text.
std::string serializeSettings(const core::Settings& settings);

/// Parses settings text leniently: unknown keys are ignored, invalid values fall back to the
/// defaults and out-of-range numbers are clamped. A broken file therefore never stops the game.
core::Settings parseSettings(std::string_view text);

/// Loads and saves the settings file.
class SettingsStore {
public:
    /// Reads and writes @p file.
    explicit SettingsStore(std::filesystem::path file);

    /// Reads the file; returns defaults if it is missing or unreadable.
    core::Settings load() const;

    /// Writes the file atomically.
    bool save(const core::Settings& settings, std::string& error) const;

private:
    std::filesystem::path m_file;
};

} // namespace io
