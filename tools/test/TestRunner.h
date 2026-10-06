#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace sp::testing {

struct TestCase {
    std::string name;
    std::string functionName;
    std::string filePath;
    int line{0};
};

struct TestResult {
    std::string name;
    std::string filePath;
    bool passed{false};
    double durationMs{0.0};
    std::string message;
};

struct TestSuiteResult {
    int total{0};
    int passed{0};
    int failed{0};
    double totalDurationMs{0.0};
    std::vector<TestResult> results;

    bool isSuccess() const { return failed == 0; }
};

class TestRunner {
public:
    explicit TestRunner(bool verbose = false);

    // Discovers test files in a directory or validates a single test file
    std::vector<std::string> discoverTestFiles(const std::string& path);

    // Extracts test blocks from a single SP file
    std::vector<TestCase> extractTestsFromFile(const std::string& filePath);

    // Runs tests in a specific file
    TestSuiteResult runFile(const std::string& filePath, const std::string& filter = "");

    // Runs tests across discovered files
    TestSuiteResult runAll(const std::string& targetPath = ".", const std::string& filter = "");

    [[nodiscard]] bool isVerbose() const { return m_verbose; }
    void setVerbose(bool v) { m_verbose = v; }

private:
    [[maybe_unused]] bool m_verbose{false};

    static std::string readFile(const std::string& path);
};

} // namespace sp::testing
