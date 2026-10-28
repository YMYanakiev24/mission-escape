#include "TestHarness.h"

#include <cstdio>
#include <string>
#include <string_view>

namespace testing {

namespace {

/// State of the test that is currently executing.
struct RunState {
    int failuresInCurrentTest = 0;
};

RunState& runState() {
    static RunState state;
    return state;
}

} // namespace

std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

void recordFailure(const char* file, int line, const std::string& message) {
    ++runState().failuresInCurrentTest;
    std::printf("    %s(%d): %s\n", file, line, message.c_str());
}

int runTests(const std::string& suiteFilter) {
    int failed = 0;
    int executed = 0;
    for (const TestCase& test : registry()) {
        if (!suiteFilter.empty() && test.suite != suiteFilter) {
            continue;
        }
        ++executed;
        runState().failuresInCurrentTest = 0;
        try {
            test.body();
        } catch (const RequireFailed&) {
            // Already recorded; the test simply stops here.
        } catch (const std::exception& error) {
            recordFailure(__FILE__, __LINE__, std::string("unexpected exception: ") + error.what());
        }
        const bool passed = runState().failuresInCurrentTest == 0;
        failed += passed ? 0 : 1;
        std::printf("[%s] %s.%s\n", passed ? " PASS " : " FAIL ", test.suite.c_str(),
                    test.name.c_str());
    }
    std::printf("\n%d test(s) run, %d passed, %d failed\n", executed, executed - failed, failed);
    if (executed == 0) {
        std::printf("No tests matched suite '%s'\n", suiteFilter.c_str());
        return 1;
    }
    return failed;
}

} // namespace testing

int main(int argc, char* argv[]) {
    std::string suite;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--suite" && index + 1 < argc) {
            suite = argv[++index];
        } else if (argument == "--list") {
            for (const testing::TestCase& test : testing::registry()) {
                std::printf("%s.%s\n", test.suite.c_str(), test.name.c_str());
            }
            return 0;
        }
    }
    return testing::runTests(suite) == 0 ? 0 : 1;
}
