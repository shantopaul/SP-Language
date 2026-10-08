#include "Benchmark.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/codegen/LLVMGenerator.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <algorithm>

namespace sp::bench {

void BenchmarkRunner::computeStatistics(const std::vector<double>& durationsNs, BenchmarkSample& sample) {
    if (durationsNs.empty()) return;

    sample.iterations = durationsNs.size();
    double sum = 0.0;
    sample.minNs = durationsNs.front();
    sample.maxNs = durationsNs.front();

    for (double d : durationsNs) {
        sum += d;
        if (d < sample.minNs) sample.minNs = d;
        if (d > sample.maxNs) sample.maxNs = d;
    }

    sample.meanNs = sum / static_cast<double>(durationsNs.size());

    // Standard deviation
    double varianceSum = 0.0;
    for (double d : durationsNs) {
        double diff = d - sample.meanNs;
        varianceSum += diff * diff;
    }
    sample.stdDevNs = std::sqrt(varianceSum / static_cast<double>(durationsNs.size()));

    // Median
    std::vector<double> sorted = durationsNs;
    std::sort(sorted.begin(), sorted.end());
    if (sorted.size() % 2 == 0) {
        sample.medianNs = (sorted[sorted.size() / 2 - 1] + sorted[sorted.size() / 2]) / 2.0;
    } else {
        sample.medianNs = sorted[sorted.size() / 2];
    }

    // Throughput (ops/sec)
    if (sample.meanNs > 0.0) {
        sample.opsPerSec = 1e9 / sample.meanNs;
    }
}

BenchmarkSample BenchmarkRunner::benchmarkCallable(const std::string& name,
                                                  const std::function<void()>& fn,
                                                  const BenchmarkOptions& options) {
    BenchmarkSample sample;
    sample.name = name;

    // 1. Warmup runs
    for (size_t i = 0; i < options.warmupRuns; ++i) {
        fn();
    }

    // 2. Sample measurement runs
    std::vector<double> durations;
    durations.reserve(options.sampleRuns);

    for (size_t i = 0; i < options.sampleRuns; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        fn();
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        durations.push_back(ns);
    }

    computeStatistics(durations, sample);
    return sample;
}

std::string BenchmarkRunner::formatTime(double ns) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    if (ns < 1000.0) {
        ss << ns << " ns";
    } else if (ns < 1000000.0) {
        ss << (ns / 1000.0) << " µs";
    } else if (ns < 1000000000.0) {
        ss << (ns / 1000000.0) << " ms";
    } else {
        ss << (ns / 1000000000.0) << " s";
    }
    return ss.str();
}

bool BenchmarkRunner::saveBaseline(const std::string& filePath, const std::vector<BenchmarkSample>& samples) {
    fs::path p(filePath);
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);

    std::ofstream out(filePath);
    if (!out.is_open()) return false;

    out << "{\n  \"benchmarks\": [\n";
    for (size_t i = 0; i < samples.size(); ++i) {
        const auto& s = samples[i];
        out << "    {\n"
            << "      \"name\": \"" << s.name << "\",\n"
            << "      \"meanNs\": " << std::fixed << std::setprecision(2) << s.meanNs << ",\n"
            << "      \"stdDevNs\": " << s.stdDevNs << "\n"
            << "    }" << (i + 1 < samples.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
    return true;
}

bool BenchmarkRunner::loadBaseline(const std::string& filePath, std::vector<BenchmarkSample>& samples) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    std::string line;
    std::string curName;
    double curMean = 0.0;
    while (std::getline(in, line)) {
        size_t namePos = line.find("\"name\": \"");
        if (namePos != std::string::npos) {
            size_t start = namePos + 9;
            size_t end = line.find('"', start);
            if (end != std::string::npos) {
                curName = line.substr(start, end - start);
            }
        }
        size_t meanPos = line.find("\"meanNs\": ");
        if (meanPos != std::string::npos) {
            std::string num = line.substr(meanPos + 10);
            while (!num.empty() && (num.back() == ',' || num.back() == ' ')) num.pop_back();
            try {
                curMean = std::stod(num);
                BenchmarkSample sample;
                sample.name = curName;
                sample.baselineMeanNs = curMean;
                sample.hasBaseline = true;
                samples.push_back(sample);
            } catch (...) {}
        }
    }
    return !samples.empty();
}

std::string BenchmarkRunner::formatReport(const std::vector<BenchmarkSample>& samples) {
    std::ostringstream out;
    out << "\n"
        << std::left << std::setw(32) << "Benchmark"
        << std::right << std::setw(8) << "Runs"
        << std::setw(14) << "Mean Time"
        << std::setw(14) << "StdDev"
        << std::setw(16) << "Throughput"
        << "   Regression Tracking\n";
    out << std::string(105, '-') << "\n";

    for (const auto& s : samples) {
        std::string regStr = "[no baseline]";
        if (s.hasBaseline && s.baselineMeanNs > 0.0) {
            double delta = s.regressionPercent;
            std::ostringstream dss;
            dss << std::fixed << std::setprecision(1);
            if (delta > 5.0) {
                dss << "+" << delta << "% SLOWER ⚠️";
            } else if (delta < -5.0) {
                dss << delta << "% faster ⚡";
            } else {
                dss << (delta >= 0 ? "+" : "") << delta << "% ~no change";
            }
            regStr = dss.str();
        }

        std::string opsStr;
        if (s.opsPerSec >= 1e6) {
            std::ostringstream opsss;
            opsss << std::fixed << std::setprecision(1) << (s.opsPerSec / 1e6) << "M ops/s";
            opsStr = opsss.str();
        } else if (s.opsPerSec >= 1e3) {
            std::ostringstream opsss;
            opsss << std::fixed << std::setprecision(1) << (s.opsPerSec / 1e3) << "k ops/s";
            opsStr = opsss.str();
        } else {
            std::ostringstream opsss;
            opsss << static_cast<long long>(s.opsPerSec) << " ops/s";
            opsStr = opsss.str();
        }

        out << std::left << std::setw(32) << s.name
            << std::right << std::setw(8) << s.iterations
            << std::setw(14) << formatTime(s.meanNs)
            << std::setw(14) << ("± " + formatTime(s.stdDevNs))
            << std::setw(16) << opsStr
            << "   " << regStr << "\n";
    }

    out << std::string(105, '-') << "\n";
    return out.str();
}

BenchmarkSuiteResult BenchmarkRunner::run(const std::string& pathOrFile, const BenchmarkOptions& options) {
    BenchmarkSuiteResult result;

    // Check if target is a file or directory
    fs::path target(pathOrFile);
    if (!fs::exists(target)) {
        result.errorMessage = "Target benchmark path does not exist: " + pathOrFile;
        return result;
    }

    // Load historical baselines if available
    std::vector<BenchmarkSample> baselines;
    loadBaseline(options.baselineFile, baselines);

    std::vector<fs::path> files;
    if (fs::is_directory(target)) {
        for (const auto& entry : fs::recursive_directory_iterator(target)) {
            if (entry.is_regular_file() && entry.path().extension() == ".sp") {
                std::string fname = entry.path().filename().string();
                if (fname.find("bench") != std::string::npos || fname.find("perf") != std::string::npos) {
                    files.push_back(entry.path());
                }
            }
        }
    } else {
        files.push_back(target);
    }

    if (files.empty()) {
        // Built-in compiler/runtime micro-benchmarks if no files match
        std::vector<BenchmarkSample> builtinSamples;

        auto s1 = benchmarkCallable("bench_lexer_throughput", []() {
            std::string code = "fn fib(n: int): int { if n <= 1 return n; return fib(n-1) + fib(n-2); }";
            std::ostringstream diag;
            diagnostics::DiagnosticEngine de(diag);
            lexer::Lexer lexer(code, "bench.sp", &de);
            volatile auto tokens = lexer.tokenizeAll();
            (void)tokens;
        }, options);
        builtinSamples.push_back(s1);

        auto s2 = benchmarkCallable("bench_parser_ast_nodes", []() {
            std::string code = "class Point { let x: int = 10; let y: int = 20; fn dist(): int { return x + y; } }";
            std::ostringstream diag;
            diagnostics::DiagnosticEngine de(diag);
            lexer::Lexer lexer(code, "bench.sp", &de);
            auto tokens = lexer.tokenizeAll();
            parser::Parser parser(tokens, "bench.sp", &de);
            volatile auto prog = parser.parseProgram();
            (void)prog;
        }, options);
        builtinSamples.push_back(s2);

        for (auto& s : builtinSamples) {
            for (const auto& b : baselines) {
                if (b.name == s.name) {
                    s.baselineMeanNs = b.baselineMeanNs;
                    s.hasBaseline = true;
                    s.regressionPercent = ((s.meanNs - b.baselineMeanNs) / b.baselineMeanNs) * 100.0;
                    if (s.regressionPercent > 10.0) {
                        result.regressionsDetected++;
                    }
                    break;
                }
            }
            result.samples.push_back(s);
        }
    } else {
        for (const auto& file : files) {
            std::string fname = file.stem().string();
            if (!options.filter.empty() && fname.find(options.filter) == std::string::npos) {
                continue;
            }

            // Benchmark compiling and parsing the file
            auto sample = benchmarkCallable(fname, [&]() {
                std::ifstream f(file);
                std::ostringstream ss;
                ss << f.rdbuf();
                std::string code = ss.str();

                std::ostringstream diag;
                diagnostics::DiagnosticEngine de(diag);
                lexer::Lexer lexer(code, file.string(), &de);
                auto tokens = lexer.tokenizeAll();
                parser::Parser parser(tokens, file.string(), &de);
                auto prog = parser.parseProgram();
                (void)prog;
            }, options);

            // Compare against baseline
            for (const auto& b : baselines) {
                if (b.name == sample.name) {
                    sample.baselineMeanNs = b.baselineMeanNs;
                    sample.hasBaseline = true;
                    sample.regressionPercent = ((sample.meanNs - b.baselineMeanNs) / b.baselineMeanNs) * 100.0;
                    if (sample.regressionPercent > 10.0) {
                        result.regressionsDetected++;
                    }
                    break;
                }
            }

            result.samples.push_back(sample);
        }
    }

    result.totalBenchmarks = result.samples.size();
    result.formattedReport = formatReport(result.samples);

    if (options.saveBaseline && !result.samples.empty()) {
        saveBaseline(options.baselineFile, result.samples);
    }

    result.success = true;
    return result;
}

} // namespace sp::bench
