#include "TestHarness.h"

#include "core/Config.h"
#include "core/Validation.h"

using namespace core;

TEST_CASE(validation, sizeNineIsRejected) {
    CHECK(!validateMazeSize("9").valid);
    CHECK(!validateMazeSize(9).valid);
}

TEST_CASE(validation, sizeTenIsAccepted) {
    const SizeValidation result = validateMazeSize("10");
    CHECK(result.valid);
    CHECK_EQ(result.size, 10);
    CHECK(result.message.empty());
}

TEST_CASE(validation, sizeFiftyIsAccepted) {
    const SizeValidation result = validateMazeSize(50);
    CHECK(result.valid);
    CHECK_EQ(result.size, 50);
}

TEST_CASE(validation, sizeFiftyOneIsRejected) {
    CHECK(!validateMazeSize("51").valid);
    CHECK(!validateMazeSize(51).valid);
}

TEST_CASE(validation, negativeSizesAreRejected) {
    CHECK(!validateMazeSize("-10").valid);
    CHECK(!validateMazeSize(-25).valid);
    CHECK(!validateMazeSize("-0").valid);
}

TEST_CASE(validation, nonNumericSizesAreRejectedWithMessage) {
    const char* inputs[] = {"abc", "12a", "1 2", "", "   ", "3.5", "+-4", "0x20", "twenty"};
    for (const char* input : inputs) {
        const SizeValidation result = validateMazeSize(input);
        CHECK(!result.valid);
        CHECK(!result.message.empty());
    }
}

TEST_CASE(validation, surroundingSpacesAndPlusSignAreTolerated) {
    CHECK(validateMazeSize("  25 ").valid);
    CHECK_EQ(validateMazeSize("+20").size, 20);
}

TEST_CASE(validation, integerOverflowIsRejected) {
    CHECK(!parseInteger("99999999999999").has_value());
    CHECK(!validateMazeSize("99999999999999").valid);
}

TEST_CASE(validation, parseIntegerAcceptsOnlyWholeNumbers) {
    CHECK_EQ(parseInteger("42").value_or(0), 42);
    CHECK_EQ(parseInteger("-7").value_or(0), -7);
    CHECK(!parseInteger("4 2").has_value());
    CHECK(!parseInteger("").has_value());
    CHECK(!parseInteger("+").has_value());
}

TEST_CASE(validation, validNamesAreAcceptedAndTrimmed) {
    const NameValidation result = validatePlayerName("  Neo 42 ");
    CHECK(result.valid);
    CHECK_EQ(result.name, std::string("Neo 42"));
    CHECK(validatePlayerName("A").valid);
    CHECK(validatePlayerName("Sixteen Chars 16").valid);
}

TEST_CASE(validation, invalidNamesAreRejected) {
    CHECK(!validatePlayerName("").valid);
    CHECK(!validatePlayerName("    ").valid);
    CHECK(!validatePlayerName("Seventeen chars 1").valid);
    CHECK(!validatePlayerName("bad=name").valid);
    CHECK(!validatePlayerName("tab\there").valid);
    CHECK(!validatePlayerName("emoji\xF0\x9F\x98\x80").valid);
}

TEST_CASE(validation, slotNumbersMustBeOneToThree) {
    CHECK(!isValidSlot(0));
    CHECK(isValidSlot(1));
    CHECK(isValidSlot(SAVE_SLOT_COUNT));
    CHECK(!isValidSlot(SAVE_SLOT_COUNT + 1));
}
