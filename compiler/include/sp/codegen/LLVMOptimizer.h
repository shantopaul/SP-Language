#pragma once

#include <string>
#include <vector>

namespace sp::codegen {

enum class OptLevel {
    O0, // No optimization
    O1, // Minimal optimization
    O2, // Standard optimization (recommended)
    O3, // Aggressive optimization
    Os, // Size optimization
    Oz  // Aggressive size optimization
};

struct OptimizerOptions {
    OptLevel level = OptLevel::O2;
    bool enableInlining = true;
    bool enableLoopUnrolling = true;
    bool enableVectorization = true;
    bool enableDCE = true;
    bool enableMem2Reg = true;
    bool enableConstantFolding = true;
    std::string customOptPath;
};

class LLVMOptimizer {
public:
    explicit LLVMOptimizer(OptimizerOptions options = {});

    /// Optimizes the input LLVM IR string according to the configured optimization level and passes.
    /// Returns true on success and writes optimized IR to outputIR.
    bool optimizeIR(const std::string& inputIR, std::string& outputIR);

    /// In-place optimization
    bool optimizeIRInPlace(std::string& ir);

    /// Runs custom in-process IR text optimization passes (constant folding, DCE, redundant load elimination).
    std::string runInProcessPasses(const std::string& inputIR) const;

    /// Returns list of LLVM optimization pass names for the given level.
    static std::vector<std::string> getPassPipeline(OptLevel level);

    /// Converts OptLevel enum to flag string ("-O0", "-O1", "-O2", "-O3", "-Os", "-Oz")
    static std::string optLevelToString(OptLevel level);

    /// Parses string ("O0", "-O1", "2", etc.) to OptLevel
    static OptLevel parseOptLevel(const std::string& str);

    const std::string& getLastError() const { return m_lastError; }
    const OptimizerOptions& getOptions() const { return m_options; }
    void setOptions(const OptimizerOptions& options) { m_options = options; }

private:
    OptimizerOptions m_options;
    std::string m_lastError;

    std::string findOptTool() const;
};

} // namespace sp::codegen
