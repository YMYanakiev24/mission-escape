#include "core/Validation.h"

#include "core/Config.h"

#include <algorithm>
#include <charconv>
#include <system_error>

namespace core {

namespace {

bool isBlank(char character) {
    return character == ' ' || character == '\t';
}

std::string sizeRangeMessage() {
    return "Size must be between " + std::to_string(MIN_MAZE_SIZE) + " and " +
           std::to_string(MAX_MAZE_SIZE) + ".";
}

} // namespace

std::string_view trim(std::string_view text) {
    while (!text.empty() && isBlank(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && isBlank(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

std::optional<int> parseInteger(std::string_view text) {
    text = trim(text);
    if (!text.empty() && text.front() == '+') {
        text.remove_prefix(1); // std::from_chars rejects an explicit plus sign
        if (!text.empty() && text.front() == '-') {
            return std::nullopt;
        }
    }
    if (text.empty()) {
        return std::nullopt;
    }
    int value = 0;
    const char* first = text.data();
    const char* last = text.data() + text.size();
    const auto [end, error] = std::from_chars(first, last, value);
    if (error != std::errc() || end != last) {
        return std::nullopt;
    }
    return value;
}

SizeValidation validateMazeSize(std::string_view text) {
    if (trim(text).empty()) {
        return SizeValidation{false, 0, "Enter a size. " + sizeRangeMessage()};
    }
    const std::optional<int> value = parseInteger(text);
    if (!value) {
        return SizeValidation{false, 0, "Size must be a whole number. " + sizeRangeMessage()};
    }
    return validateMazeSize(*value);
}

SizeValidation validateMazeSize(int value) {
    if (value < MIN_MAZE_SIZE || value > MAX_MAZE_SIZE) {
        return SizeValidation{false, 0, sizeRangeMessage()};
    }
    return SizeValidation{true, value, ""};
}

bool isNameCharacter(int codepoint) {
    const bool lower = codepoint >= 'a' && codepoint <= 'z';
    const bool upper = codepoint >= 'A' && codepoint <= 'Z';
    const bool digit = codepoint >= '0' && codepoint <= '9';
    return lower || upper || digit || codepoint == ' ';
}

NameValidation validatePlayerName(std::string_view text) {
    const std::string_view trimmed = trim(text);
    if (trimmed.empty()) {
        return NameValidation{false, "", "Enter a name."};
    }
    if (trimmed.size() > static_cast<std::size_t>(PLAYER_NAME_MAX_LENGTH)) {
        return NameValidation{false, "",
                              "Names can be at most " + std::to_string(PLAYER_NAME_MAX_LENGTH) +
                                  " characters."};
    }
    const bool allAllowed = std::all_of(trimmed.begin(), trimmed.end(), [](char character) {
        return isNameCharacter(static_cast<unsigned char>(character));
    });
    if (!allAllowed) {
        return NameValidation{false, "", "Use letters, digits and spaces only."};
    }
    return NameValidation{true, std::string(trimmed), ""};
}

bool isValidSlot(int slot) {
    return slot >= 1 && slot <= SAVE_SLOT_COUNT;
}

int clampInt(int value, int minimum, int maximum) {
    return std::clamp(value, minimum, maximum);
}

} // namespace core
