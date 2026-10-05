#include "sp/ir/IRPrinter.h"
#include <iomanip>

namespace sp::ir {

std::string IRPrinter::print(const IRModule& module) {
    std::ostringstream ss;
    ss << "; ModuleID = '" << module.getName() << "'\n";
    ss << "source_filename = \"" << module.getName() << ".sp\"\n\n";

    // String literals
    if (!module.getStringLiterals().empty()) {
        ss << "; String Constants\n";
        for (const auto& lit : module.getStringLiterals()) {
            size_t len = lit.content.length() + 1; // including null terminator
            ss << lit.name << " = private unnamed_addr constant [" << len << " x i8] c\"";
            for (char c : lit.content) {
                if (c == '\n') ss << "\\0A";
                else if (c == '\t') ss << "\\09";
                else if (c == '\r') ss << "\\0D";
                else if (c == '\\') ss << "\\5C";
                else if (c == '\"') ss << "\\22";
                else ss << c;
            }
            ss << "\\00\", align 1\n";
        }
        ss << "\n";
    }

    // Struct / Class Types
    if (!module.getStructTypes().empty()) {
        ss << "; Struct / Class Types\n";
        for (const auto& st : module.getStructTypes()) {
            ss << "%" << st.name << " = type { ";
            for (size_t i = 0; i < st.elementTypes.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << (st.elementTypes[i] ? st.elementTypes[i]->toString() : "ptr");
            }
            ss << " }\n";
        }
        ss << "\n";
    }

    // Global Constants and VTables
    if (!module.getGlobalVariables().empty()) {
        ss << "; Global Constants and VTables\n";
        for (const auto& g : module.getGlobalVariables()) {
            ss << g.name << " = " << (g.isConstant ? "constant " : "global ")
               << g.type->toString() << " " << g.rawValue << ", align 8\n";
        }
        ss << "\n";
    }

    // Function declarations and definitions
    for (const auto& func : module.getFunctions()) {
        printFunction(*func, ss);
    }

    return ss.str();
}

std::string IRPrinter::print(const IRFunction& function) {
    std::ostringstream ss;
    printFunction(function, ss);
    return ss.str();
}

std::string IRPrinter::print(const IRBasicBlock& block) {
    std::ostringstream ss;
    printBasicBlock(block, ss);
    return ss.str();
}

std::string IRPrinter::print(const IRInstruction& instruction) {
    std::ostringstream ss;
    printInstruction(instruction, ss);
    return ss.str();
}

void IRPrinter::printFunction(const IRFunction& function, std::ostringstream& ss) {
    if (function.isExternal()) {
        ss << "declare " << function.getReturnType()->toString() << " @" << function.getName() << "(";
        const auto& params = function.getParams();
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << params[i]->getType()->toString();
        }
        ss << ")\n\n";
        return;
    }

    ss << "define " << function.getReturnType()->toString() << " @" << function.getName() << "(";
    const auto& params = function.getParams();
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << params[i]->getType()->toString() << " " << params[i]->getName();
    }
    ss << ") {\n";

    for (const auto& block : function.getBlocks()) {
        printBasicBlock(*block, ss);
    }

    ss << "}\n\n";
}

void IRPrinter::printBasicBlock(const IRBasicBlock& block, std::ostringstream& ss) {
    ss << block.getName() << ":\n";
    for (const auto& inst : block.getInstructions()) {
        ss << "    ";
        printInstruction(*inst, ss);
        ss << "\n";
    }
}

void IRPrinter::printInstruction(const IRInstruction& inst, std::ostringstream& ss) {
    ss << inst.format();
}

} // namespace sp::ir
