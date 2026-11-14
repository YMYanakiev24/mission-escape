#include "io/KeyValueDocument.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iterator>
#include <system_error>

namespace io {

namespace {

constexpr std::size_t MAX_KEY_LENGTH = 64;
constexpr std::size_t MAX_VALUE_LENGTH = 4096;
constexpr unsigned char FIRST_PRINTABLE = 32;
constexpr unsigned char DELETE_CHARACTER = 127;
constexpr const char* TEMPORARY_SUFFIX = ".tmp";

bool isKeyCharacter(char character) {
    const bool lower = character >= 'a' && character <= 'z';
    const bool upper = character >= 'A' && character <= 'Z';
    const bool digit = character >= '0' && character <= '9';
    return lower || upper || digit || character == '.' || character == '_';
}

/// Removes a trailing '\r' so files edited on Windows still parse.
std::string_view withoutCarriageReturn(std::string_view line) {
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }
    return line;
}

} // namespace

bool KeyValueDocument::isValidKey(std::string_view key) {
    return !key.empty() && key.size() <= MAX_KEY_LENGTH &&
           std::all_of(key.begin(), key.end(), isKeyCharacter);
}

bool KeyValueDocument::isValidValue(std::string_view value) {
    return value.size() <= MAX_VALUE_LENGTH &&
           std::all_of(value.begin(), value.end(), [](char character) {
               const auto code = static_cast<unsigned char>(character);
               return code >= FIRST_PRINTABLE && code != DELETE_CHARACTER;
           });
}

bool KeyValueDocument::set(const std::string& key, const std::string& value) {
    if (!isValidKey(key) || !isValidValue(value)) {
        return false;
    }
    for (auto& entry : m_entries) {
        if (entry.first == key) {
            entry.second = value;
            return true;
        }
    }
    m_entries.emplace_back(key, value);
    return true;
}

bool KeyValueDocument::setInt(const std::string& key, long long value) {
    return set(key, std::to_string(value));
}

bool KeyValueDocument::setBool(const std::string& key, bool value) {
    return set(key, value ? "1" : "0");
}

bool KeyValueDocument::has(std::string_view key) const {
    return get(key).has_value();
}

std::optional<std::string> KeyValueDocument::get(std::string_view key) const {
    for (const auto& entry : m_entries) {
        if (entry.first == key) {
            return entry.second;
        }
    }
    return std::nullopt;
}

std::optional<long long> KeyValueDocument::getInt(std::string_view key, long long minimum,
                                                  long long maximum) const {
    const std::optional<std::string> text = get(key);
    if (!text) {
        return std::nullopt;
    }
    const std::optional<long long> value = parseLongLong(*text);
    if (!value || *value < minimum || *value > maximum) {
        return std::nullopt;
    }
    return value;
}

std::optional<bool> KeyValueDocument::getBool(std::string_view key) const {
    const std::optional<std::string> text = get(key);
    if (text == "1") {
        return true;
    }
    if (text == "0") {
        return false;
    }
    return std::nullopt;
}

std::string KeyValueDocument::serialize() const {
    std::string text;
    for (const auto& entry : m_entries) {
        text += entry.first + "=" + entry.second + "\n";
    }
    return text;
}

std::optional<KeyValueDocument> KeyValueDocument::parse(std::string_view text, std::string& error) {
    KeyValueDocument document;
    int lineNumber = 0;
    while (!text.empty()) {
        ++lineNumber;
        const std::size_t end = text.find('\n');
        const std::string_view line =
            withoutCarriageReturn(end == std::string_view::npos ? text : text.substr(0, end));
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
        if (line.empty()) {
            continue;
        }
        const std::size_t separator = line.find('=');
        if (separator == std::string_view::npos) {
            error = "line " + std::to_string(lineNumber) + " has no '='";
            return std::nullopt;
        }
        const std::string key(line.substr(0, separator));
        const std::string value(line.substr(separator + 1));
        if (!isValidKey(key) || !isValidValue(value)) {
            error = "line " + std::to_string(lineNumber) + " has an invalid key or value";
            return std::nullopt;
        }
        if (document.has(key)) {
            error = "duplicate key '" + key + "'";
            return std::nullopt;
        }
        document.m_entries.emplace_back(key, value);
    }
    return document;
}

std::optional<long long> parseLongLong(std::string_view text) {
    if (text.empty()) {
        return std::nullopt;
    }
    long long value = 0;
    const char* last = text.data() + text.size();
    const auto [end, result] = std::from_chars(text.data(), last, value);
    if (result != std::errc() || end != last) {
        return std::nullopt;
    }
    return value;
}

std::vector<std::string> splitList(std::string_view text, char separator) {
    std::vector<std::string> parts;
    if (text.empty()) {
        return parts;
    }
    std::size_t start = 0;
    while (true) {
        const std::size_t end = text.find(separator, start);
        parts.emplace_back(
            text.substr(start, end == std::string_view::npos ? text.size() - start : end - start));
        if (end == std::string_view::npos) {
            return parts;
        }
        start = end + 1;
    }
}

std::optional<std::string> readTextFile(const std::filesystem::path& path, std::uintmax_t maxBytes,
                                        std::string& error) {
    std::error_code code;
    if (!std::filesystem::is_regular_file(path, code)) {
        error = "file not found";
        return std::nullopt;
    }
    const std::uintmax_t size = std::filesystem::file_size(path, code);
    if (code || size > maxBytes) {
        error = code ? "cannot read file size" : "file is too large";
        return std::nullopt;
    }
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "cannot open file";
        return std::nullopt;
    }
    std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (stream.bad()) {
        error = "read error";
        return std::nullopt;
    }
    return text;
}

bool writeTextFileAtomically(const std::filesystem::path& path, const std::string& text,
                             std::string& error) {
    std::error_code code;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), code);
        if (code) {
            error = "cannot create folder: " + code.message();
            return false;
        }
    }
    std::filesystem::path temporary = path;
    temporary += TEMPORARY_SUFFIX;
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        stream << text;
        stream.flush();
        if (!stream) {
            error = "cannot write temporary file";
            return false;
        }
    }
    std::filesystem::rename(temporary, path, code);
    if (code) {
        std::filesystem::remove(temporary, code);
        error = "cannot replace file";
        return false;
    }
    return true;
}

} // namespace io
