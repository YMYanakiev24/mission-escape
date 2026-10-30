#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/// @file KeyValueDocument.h
/// The `key=value` text format shared by save files and settings.cfg, plus safe file I/O.

namespace io {

/// An ordered list of unique `key=value` entries.
///
/// Keys may contain letters, digits, '.' and '_'; values may contain any printable
/// characters except line breaks. Parsing is strict: malformed lines or duplicate keys
/// make the whole document invalid, which is how corrupted files are detected.
class KeyValueDocument {
public:
    /// Adds or replaces an entry. Invalid keys or values are ignored (and reported false).
    bool set(const std::string& key, const std::string& value);

    /// Stores an integer.
    bool setInt(const std::string& key, long long value);

    /// Stores a boolean as "1" / "0".
    bool setBool(const std::string& key, bool value);

    /// True when @p key is present.
    bool has(std::string_view key) const;

    /// Raw value of @p key.
    std::optional<std::string> get(std::string_view key) const;

    /// Integer value of @p key within [minimum, maximum]; nothing if absent or invalid.
    std::optional<long long> getInt(std::string_view key, long long minimum,
                                    long long maximum) const;

    /// Boolean value of @p key ("0" or "1" only).
    std::optional<bool> getBool(std::string_view key) const;

    /// Number of entries.
    std::size_t size() const noexcept { return m_entries.size(); }

    /// Serialises the document as `key=value` lines.
    std::string serialize() const;

    /// Parses a document; on failure returns nothing and describes the problem in @p error.
    static std::optional<KeyValueDocument> parse(std::string_view text, std::string& error);

    /// True for keys made of letters, digits, '.' and '_'.
    static bool isValidKey(std::string_view key);

    /// True for values without control characters.
    static bool isValidValue(std::string_view value);

private:
    std::vector<std::pair<std::string, std::string>> m_entries;
};

/// Strict integer parsing of a whole string (optional '-' then digits).
std::optional<long long> parseLongLong(std::string_view text);

/// Splits @p text at @p separator (an empty text yields no parts).
std::vector<std::string> splitList(std::string_view text, char separator);

/// Reads a whole text file if it exists and is at most @p maxBytes long.
std::optional<std::string> readTextFile(const std::filesystem::path& path, std::uintmax_t maxBytes,
                                        std::string& error);

/// Writes @p text to a temporary file next to @p path and renames it over @p path, so a crash
/// can never leave a half-written file. Creates the parent folder when needed.
bool writeTextFileAtomically(const std::filesystem::path& path, const std::string& text,
                             std::string& error);

} // namespace io
