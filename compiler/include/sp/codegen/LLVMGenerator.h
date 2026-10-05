#pragma once

#include "sp/ir/IRModule.h"
#include <string>
#include <vector>
#include <memory>

namespace sp::codegen {

struct CodegenOptions {
    std::string targetTriple;
    std::string dataLayout;
    bool releaseMode{false};
    bool debugSymbols{false};
    std::string clangPath;
    std::string runtimeSourcePath;
    std::string runtimeIncludePath;
};

class LLVMGenerator {
public:
    explicit LLVMGenerator(CodegenOptions options = {});

    // Generate LLVM IR text (.ll) from an SP IR Module
    [[nodiscard]] std::string generateLLVMIR(const ir::IRModule& module);

    // Write LLVM IR text directly to file
    bool emitLLVMIRToFile(const ir::IRModule& module, const std::string& filePath);

    // Compile an SP IR module directly to a native standalone executable (.exe / ELF)
    bool compileToExecutable(const ir::IRModule& module, const std::string& outputExePath);

    // Locate Clang compiler executable on host machine
    [[nodiscard]] static std::string findClangCompiler();

    // Locate SP Runtime source file (Runtime.cpp)
    [[nodiscard]] static std::string findRuntimeSourcePath();

    // Locate SP Runtime include directory
    [[nodiscard]] static std::string findRuntimeIncludePath();

    // Host platform triple detection
    [[nodiscard]] static std::string getHostTargetTriple();

    [[nodiscard]] const std::string& getLastError() const noexcept { return m_lastError; }

private:
    CodegenOptions m_options;
    std::string m_lastError;
};

} // namespace sp::codegen
