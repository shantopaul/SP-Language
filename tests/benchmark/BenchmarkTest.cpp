#include <gtest/gtest.h>
#include "benchmark/Benchmark.h"
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
using namespace sp::bench;

class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_tempBaseline = fs::temp_directory_path() / ("sp_bench_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".json");
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove(m_tempBaseline, ec);
    }

    fs::path m_tempBaseline;
};

// 1. Statistics Computation Test
TEST_F(BenchmarkTest, ComputeStatisticsAccurateMath) {
    std::vector<double> durations = {100.0, 200.0, 300.0, 400.0, 500.0};
    BenchmarkSample sample;
    BenchmarkRunner::computeStatistics(durations, sample);

    EXPECT_EQ(sample.iterations, 5);
    EXPECT_DOUBLE_EQ(sample.minNs, 100.0);
    EXPECT_DOUBLE_EQ(sample.maxNs, 500.0);
    EXPECT_DOUBLE_EQ(sample.meanNs, 300.0);
    EXPECT_DOUBLE_EQ(sample.medianNs, 300.0);
    EXPECT_GT(sample.stdDevNs, 140.0);
    EXPECT_LT(sample.stdDevNs, 142.0);
    EXPECT_DOUBLE_EQ(sample.opsPerSec, 1e9 / 300.0);
}

// 2. Benchmark Callable Execution
TEST_F(BenchmarkTest, BenchmarkCallableExecutionAndFormat) {
    BenchmarkOptions opts;
    opts.warmupRuns = 2;
    opts.sampleRuns = 10;

    int counter = 0;
    auto sample = BenchmarkRunner::benchmarkCallable("test_counter_inc", [&]() {
        counter++;
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }, opts);

    EXPECT_EQ(sample.name, "test_counter_inc");
    EXPECT_EQ(sample.iterations, 10);
    EXPECT_GT(sample.meanNs, 0.0);
    EXPECT_GT(sample.opsPerSec, 0.0);

    std::string formatted = BenchmarkRunner::formatTime(sample.meanNs);
    EXPECT_FALSE(formatted.empty());
}

// 3. Baseline Persistence and Regression Detection
TEST_F(BenchmarkTest, BaselinePersistenceAndRegressionDetection) {
    BenchmarkSample s1;
    s1.name = "bench_sha256";
    s1.meanNs = 1000.0;
    s1.stdDevNs = 50.0;

    std::vector<BenchmarkSample> toSave = {s1};
    EXPECT_TRUE(BenchmarkRunner::saveBaseline(m_tempBaseline.string(), toSave));
    EXPECT_TRUE(fs::exists(m_tempBaseline));

    std::vector<BenchmarkSample> loaded;
    EXPECT_TRUE(BenchmarkRunner::loadBaseline(m_tempBaseline.string(), loaded));
    ASSERT_EQ(loaded.size(), 1);
    EXPECT_EQ(loaded[0].name, "bench_sha256");
    EXPECT_DOUBLE_EQ(loaded[0].baselineMeanNs, 1000.0);

    // Test regression calculation
    // If new run takes 1300.0 ns, delta is +30% (regression > 10%)
    double newMean = 1300.0;
    double delta = ((newMean - loaded[0].baselineMeanNs) / loaded[0].baselineMeanNs) * 100.0;
    EXPECT_DOUBLE_EQ(delta, 30.0);
}
