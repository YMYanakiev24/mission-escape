#pragma once

#include "core/Repositories.h"
#include "core/SaveData.h"

#include <filesystem>
#include <string>
#include <string_view>

/// @file SaveSystem.h
/// Data access layer: save slots stored as `saves/slotN.sav` key=value text files.

namespace io {

/// The load result types belong to the business layer's repository contract.
using LoadStatus = core::LoadStatus;
using LoadResult = core::LoadResult;

/// Serialises a profile (including an optional level snapshot) to save-file text.
std::string serializeSave(const core::SaveData& data);

/// Parses and fully validates save-file text. Any invalid field makes the result Corrupt;
/// the function never throws and never crashes on malformed input.
LoadResult parseSave(std::string_view text);

/// Reads and writes the three save slots inside one folder (implements core::SaveRepository).
class SaveSystem final : public core::SaveRepository {
public:
    /// Uses @p directory (created on first save if missing).
    explicit SaveSystem(std::filesystem::path directory);

    /// Path of slot 1..3.
    std::filesystem::path slotPath(int slot) const;

    /// Loads slot 1..3.
    LoadResult load(int slot) const override;

    /// Writes slot 1..3 atomically; returns false and fills @p error on failure.
    bool save(int slot, const core::SaveData& data, std::string& error) const override;

    /// Deletes slot 1..3; returns true if the slot no longer exists afterwards.
    bool erase(int slot) const override;

private:
    std::filesystem::path m_directory;
};

} // namespace io
