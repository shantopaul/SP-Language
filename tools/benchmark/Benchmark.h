#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <functional>

namespace sp::bench {

namespace fs = std::filesystem;

struct BenchmarkSample {
    std::string name;
    size_t iterations{0};
    double minNs{0.0};
    double maxNs{0.0};
    double meanNs{0.0};
    double medianNs{0.0};
    double stdDevNs{0.0};
    double opsPerSec{0.0};
    double baselineMeanNs{0.0};
    double regressionPercent{0.0};
    bool hasBaseline{false};
};

struct BenchmarkOptions {
    size_t warmupRuns{5};
    size_t sampleRuns{50};
    bool saveBaseline{false};
    std::string baselineFile{".sp/benchmarks/baseline.json"};
    std::string filter;
};

struct BenchmarkSuiteResult {
    bool success{false};
    std::vector<BenchmarkSample> samples;
    size_t totalBenchmarks{0};
    size_t regressionsDetected{0};
    std::string formattedReport;
    std::string errorMessage;
};

class BenchmarkRunner {
public:
    static BenchmarkSuiteResult run(const std::string& pathOrFile, const BenchmarkOptions& options = {});

    // Benchmark a specific C++ or compiled callable
    static BenchmarkSample benchmarkCallable(const std::string& name,
                                             const std::function<void()>& fn,
                                             const BenchmarkOptions& options = {});

    // Statistical calculations
    static void computeStatistics(const std::vector<double>& durationsNs, BenchmarkSample& sample);

    // Save and load baseline tracking
    static bool saveBaseline(const std::string& filePath, const std::vector<BenchmarkSample>& samples);
    static bool loadBaseline(const std::string& filePath, std::vector<BenchmarkSample>& samples);

    // Format console output
    static std::string formatReport(const std::vector<BenchmarkSample>& samples);
    static std::string formatTime(double ns);
};

} // namespace sp::bench
