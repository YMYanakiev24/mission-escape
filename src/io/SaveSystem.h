#pragma once

#include "core/SaveData.h"

#include <filesystem>
#include <string>
#include <string_view>

/// @file SaveSystem.h
/// Save slots stored as `saves/slotN.sav` key=value text files.

namespace io {

/// Outcome of reading a save slot.
enum class LoadStatus { Ok, Missing, Corrupt };

/// A loaded slot, or the reason it could not be loaded.
struct LoadResult {
    LoadStatus status = LoadStatus::Missing;
    core::SaveData data;
    std::string error; ///< Human readable reason when status is Corrupt.
};

/// Serialises a profile (including an optional level snapshot) to save-file text.
std::string serializeSave(const core::SaveData& data);

/// Parses and fully validates save-file text. Any invalid field makes the result Corrupt;
/// the function never throws and never crashes on malformed input.
LoadResult parseSave(std::string_view text);

/// Current local date and time as "YYYY-MM-DD HH:MM".
std::string currentTimestamp();

/// Reads and writes the three save slots inside one folder.
class SaveSystem {
public:
    /// Uses @p directory (created on first save if missing).
    explicit SaveSystem(std::filesystem::path directory);

    /// Path of slot 1..3.
    std::filesystem::path slotPath(int slot) const;

    /// Loads slot 1..3.
    LoadResult load(int slot) const;

    /// Writes slot 1..3 atomically; returns false and fills @p error on failure.
    bool save(int slot, const core::SaveData& data, std::string& error) const;

    /// Deletes slot 1..3; returns true if the slot no longer exists afterwards.
    bool erase(int slot) const;

private:
    std::filesystem::path m_directory;
};

} // namespace io
