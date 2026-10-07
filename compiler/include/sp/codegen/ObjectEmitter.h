#pragma once

#include "sp/ir/IRModule.h"
#include <string>
#include <vector>
#include <cstdint>

namespace sp::codegen {

struct ObjectEmitterOptions {
    std::string targetTriple;
    std::string dataLayout;
    std::string optLevel{"-O2"}; // -O0, -O1, -O2, -O3, -Os, -Oz
    bool pic{false};             // Position-independent code
    std::string codeModel{"default"};
    bool debugSymbols{false};
    std::string llvmToolsPath;   // Optional path override for LLVM tools
};

class ObjectEmitter {
public:
    explicit ObjectEmitter(ObjectEmitterOptions options = {});

    // Emits a relocatable native object file (.o / .obj) from an SP IR Module
    [[nodiscard]] bool emitObjectFile(const ir::IRModule& module, const std::string& outputObjPath);

    // Emits a relocatable native object file (.o / .obj) directly from LLVM IR text
    [[nodiscard]] bool emitObjectFromIR(const std::string& llvmIR, const std::string& outputObjPath);

    // Emits a native object directly into an in-memory byte buffer
    [[nodiscard]] bool emitObjectToMemory(const ir::IRModule& module, std::vector<uint8_t>& outBuffer);

    // Emits a native object from LLVM IR text directly into an in-memory byte buffer
    [[nodiscard]] bool emitObjectFromIRToMemory(const std::string& llvmIR, std::vector<uint8_t>& outBuffer);

    // Locate LLVM tool (clang, llc, opt, etc.)
    [[nodiscard]] static std::string findLLVMTool(const std::string& toolName, const std::string& customPath = "");

    // Platform default object extension (.obj on MSVC, .o on GNU/POSIX)
    [[nodiscard]] static std::string getObjectFileExtension(const std::string& targetTriple);

    // Detect host target architecture triple
    [[nodiscard]] static std::string getHostTriple();

    [[nodiscard]] const std::string& getLastError() const noexcept { return m_lastError; }

private:
    ObjectEmitterOptions m_options;
    std::string m_lastError;
};

} // namespace sp::codegen
