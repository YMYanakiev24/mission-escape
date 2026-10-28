#pragma once

#include <exception>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

/// @file TestHarness.h
/// A minimal, dependency-free unit test harness.
///
/// Usage:
/// @code
/// TEST_CASE(generation, perfectMazeConnectsEveryCell) {
///     CHECK(allCellsReachable(maze, rules));   // records a failure, keeps running
///     CHECK_EQ(maze.size(), 10);                // prints both values on failure
///     REQUIRE(!path.empty());                   // stops this test on failure
/// }
/// @endcode
/// Every TEST_CASE registers itself before main() runs; test_main.cpp executes the registry.

namespace testing {

/// Signature of a test body.
using TestFunction = void (*)();

/// One registered test.
struct TestCase {
    std::string suite;
    std::string name;
    TestFunction body = nullptr;
};

/// The list of all registered tests (function-local static, filled during static init).
std::vector<TestCase>& registry();

/// Records a failed check in the currently running test.
void recordFailure(const char* file, int line, const std::string& message);

/// Thrown by REQUIRE to abort the current test after a failed precondition.
class RequireFailed : public std::exception {
public:
    const char* what() const noexcept override { return "REQUIRE failed"; }
};

/// Adds a test to the registry at static initialisation time.
struct Registrar {
    Registrar(const char* suite, const char* name, TestFunction body) {
        registry().push_back(TestCase{suite, name, body});
    }
};

/// Formats a value for failure messages (enumerations print their underlying number).
template <typename T>
std::string describe(const T& value) {
    std::ostringstream stream;
    if constexpr (std::is_enum_v<T>) {
        stream << static_cast<long long>(value);
    } else {
        stream << value;
    }
    return stream.str();
}

/// Runs every test whose suite matches @p suiteFilter (empty = all); returns failed count.
int runTests(const std::string& suiteFilter);

} // namespace testing

#define TEST_CONCAT_INNER(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_INNER(a, b)

/// Defines and registers a test function `name` in group `suite`.
#define TEST_CASE(suite, name)                                                                     \
    static void name();                                                                            \
    static const testing::Registrar TEST_CONCAT(registrar_, name)(#suite, #name, &name);           \
    static void name()

/// Non-fatal check: records a failure and continues.
#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            testing::recordFailure(__FILE__, __LINE__, "CHECK(" #condition ")");                   \
        }                                                                                          \
    } while (false)

/// Non-fatal equality check that prints both values on failure.
#define CHECK_EQ(actual, expected)                                                                 \
    do {                                                                                           \
        const auto& neonActual = (actual);                                                         \
        const auto& neonExpected = (expected);                                                     \
        if (!(neonActual == neonExpected)) {                                                       \
            testing::recordFailure(__FILE__, __LINE__,                                             \
                                   "CHECK_EQ(" #actual ", " #expected ") got " +                   \
                                       testing::describe(neonActual) + ", expected " +             \
                                       testing::describe(neonExpected));                           \
        }                                                                                          \
    } while (false)

/// Fatal check: records a failure and stops the current test.
#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            testing::recordFailure(__FILE__, __LINE__, "REQUIRE(" #condition ")");                 \
            throw testing::RequireFailed();                                                        \
        }                                                                                          \
    } while (false)
