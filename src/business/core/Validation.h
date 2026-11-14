#pragma once

#include <optional>
#include <string>
#include <string_view>

/// @file Validation.h
/// Validation of everything the user (or a file) can feed into the game.

namespace core {

/// Outcome of validating a maze size typed by the player.
struct SizeValidation {
    bool valid = false;
    int size = 0;        ///< The accepted size (only meaningful when valid).
    std::string message; ///< Human readable reason when invalid, empty otherwise.
};

/// Outcome of validating a player name.
struct NameValidation {
    bool valid = false;
    std::string name;    ///< The trimmed name (only meaningful when valid).
    std::string message; ///< Human readable reason when invalid, empty otherwise.
};

/// Removes leading and trailing spaces and tabs.
std::string_view trim(std::string_view text);

/// Strict integer parser: optional sign followed by digits only (surrounding spaces allowed).
/// Returns nothing for empty text, stray characters or values outside the `int` range.
std::optional<int> parseInteger(std::string_view text);

/// Validates a maze size given as text (e.g. from the numeric text field).
SizeValidation validateMazeSize(std::string_view text);

/// Validates a maze size given as a number (e.g. from the slider or a save file).
SizeValidation validateMazeSize(int value);

/// True for the characters allowed in player names: ASCII letters, digits and spaces.
bool isNameCharacter(int codepoint);

/// Validates a player name: 1-16 characters after trimming, letters, digits and spaces only.
NameValidation validatePlayerName(std::string_view text);

/// True for save slot numbers 1..SAVE_SLOT_COUNT.
bool isValidSlot(int slot);

/// Clamps @p value into [minimum, maximum].
int clampInt(int value, int minimum, int maximum);

} // namespace core
