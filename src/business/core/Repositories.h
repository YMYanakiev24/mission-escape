#pragma once

#include "core/SaveData.h"
#include "core/Settings.h"

#include <string>

/// @file Repositories.h
/// The contracts between the business layer and the data access layer. The business layer
/// only knows these interfaces; the data layer (src/data) implements them with files. That
/// keeps the dependency pointing from the data layer to the business layer, never back.

namespace core {

/// Outcome of reading a save slot.
enum class LoadStatus { Ok, Missing, Corrupt };

/// A loaded slot, or the reason it could not be loaded.
struct LoadResult {
    LoadStatus status = LoadStatus::Missing;
    SaveData data;
    std::string error; ///< Human readable reason when status is Corrupt.
};

/// Storage of the three save slots (implemented by io::SaveSystem).
class SaveRepository {
public:
    virtual ~SaveRepository() = default;

    /// Reads slot 1..3.
    virtual LoadResult load(int slot) const = 0;

    /// Writes slot 1..3; returns false and fills @p error on failure.
    virtual bool save(int slot, const SaveData& data, std::string& error) const = 0;

    /// Deletes slot 1..3; returns true if the slot no longer exists afterwards.
    virtual bool erase(int slot) const = 0;

protected:
    SaveRepository() = default;
    SaveRepository(const SaveRepository&) = default;
    SaveRepository& operator=(const SaveRepository&) = default;
};

/// Storage of the user settings (implemented by io::SettingsStore).
class SettingsRepository {
public:
    virtual ~SettingsRepository() = default;

    /// Reads the settings; returns the defaults if nothing usable is stored.
    virtual Settings load() const = 0;

    /// Writes the settings; returns false and fills @p error on failure.
    virtual bool save(const Settings& settings, std::string& error) const = 0;

protected:
    SettingsRepository() = default;
    SettingsRepository(const SettingsRepository&) = default;
    SettingsRepository& operator=(const SettingsRepository&) = default;
};

} // namespace core
