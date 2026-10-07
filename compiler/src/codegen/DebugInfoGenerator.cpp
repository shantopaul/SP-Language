#include "sp/codegen/DebugInfoGenerator.h"
#include <sstream>
#include <regex>

namespace sp::codegen {

DebugInfoGenerator::DebugInfoGenerator(std::string sourceFilename, std::string sourceDirectory)
    : m_sourceFilename(std::move(sourceFilename)), m_sourceDirectory(std::move(sourceDirectory)) {
}

void DebugInfoGenerator::setSourceFile(std::string filename, std::string directory) {
    m_sourceFilename = std::move(filename);
    m_sourceDirectory = std::move(directory);
}

int DebugInfoGenerator::createSubprogram(const std::string& name, const std::string& linkageName, uint32_t line) {
    int id = static_cast<int>(m_subprograms.size());
    m_subprograms.push_back({
        id,
        name,
        linkageName.empty() ? name : linkageName,
        line == 0 ? 1 : line,
        line == 0 ? 1 : line,
        0 // primary file
    });
    return id;
}

int DebugInfoGenerator::createLocation(uint32_t line, uint32_t column, int subprogramId) {
    int id = static_cast<int>(m_locations.size());
    m_locations.push_back({
        line == 0 ? 1 : line,
        column == 0 ? 1 : column,
        subprogramId
    });
    return id;
}

std::string DebugInfoGenerator::generateMetadataProlog() const {
    std::ostringstream ss;
    ss << "!llvm.dbg.cu = !{!0}\n";
    ss << "!llvm.module.flags = !{!1, !2}\n\n";
    ss << "!1 = !{i32 2, !\"Dwarf Version\", i32 5}\n";
    ss << "!2 = !{i32 2, !\"Debug Info Version\", i32 3}\n\n";
    return ss.str();
}

std::string DebugInfoGenerator::generateMetadataEpilog() const {
    std::ostringstream ss;
    ss << "\n; --- Debug Information Metadata ---\n";
    ss << "!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: \"spc 1.0.0 (SP Compiler)\", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)\n";
    ss << "!3 = !DIFile(filename: \"" << m_sourceFilename << "\", directory: \"" << m_sourceDirectory << "\")\n";
    ss << "!4 = !DISubroutineType(types: !5)\n";
    ss << "!5 = !{null}\n";

    // Base offset for subprograms is 10
    int subprogBase = 10;
    for (size_t i = 0; i < m_subprograms.size(); ++i) {
        const auto& sp = m_subprograms[i];
        int spNodeId = subprogBase + static_cast<int>(i);
        ss << "!" << spNodeId << " = distinct !DISubprogram(name: \"" << sp.name
           << "\", linkageName: \"" << sp.linkageName
           << "\", scope: !3, file: !3, line: " << sp.line
           << ", type: !4, scopeLine: " << sp.scopeLine
           << ", flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0)\n";
    }

    // Base offset for locations is 100
    int locBase = 100;
    for (size_t i = 0; i < m_locations.size(); ++i) {
        const auto& loc = m_locations[i];
        int locNodeId = locBase + static_cast<int>(i);
        int scopeNodeId = (loc.subprogramId >= 0 && static_cast<size_t>(loc.subprogramId) < m_subprograms.size())
            ? (subprogBase + loc.subprogramId)
            : 0;
        ss << "!" << locNodeId << " = !DILocation(line: " << loc.line
           << ", column: " << loc.column
           << ", scope: !" << scopeNodeId << ")\n";
    }

    return ss.str();
}

std::string DebugInfoGenerator::injectDebugInfo(const std::string& llvmIR) {
    // If no subprograms are registered, scan for function definitions and auto-register them
    std::istringstream stream(llvmIR);
    std::string line;
    std::ostringstream header;
    std::ostringstream body;

    std::regex fnDefRegex(R"(^define\s+.*@([A-Za-z0-9_]+)\s*\()");
    std::smatch match;

    int currentSubprogId = -1;
    uint32_t currentLineNum = 1;
    bool pastHeader = false;

    while (std::getline(stream, line)) {
        // Any target datalayout or target triple directives must precede metadata
        if (!pastHeader && (line.rfind("target datalayout", 0) == 0 ||
                            line.rfind("target triple", 0) == 0 ||
                            line.rfind("source_filename", 0) == 0 ||
                            line.empty())) {
            header << line << "\n";
            continue;
        }
        pastHeader = true;

        if (std::regex_search(line, match, fnDefRegex)) {
            std::string fnName = match[1].str();
            currentSubprogId = createSubprogram(fnName, fnName, currentLineNum);
            // Append !dbg attachment to function definition before opening brace
            int spNodeId = 10 + currentSubprogId;
            auto bracePos = line.rfind('{');
            if (bracePos != std::string::npos) {
                line.insert(bracePos, "!dbg !" + std::to_string(spNodeId) + " ");
            } else {
                line += " !dbg !" + std::to_string(spNodeId);
            }
        } else if (currentSubprogId >= 0 && !line.empty() && line[0] == ' ' && line.find('=') != std::string::npos) {
            // Instruction inside a function: tag with debug location if not already tagged
            if (line.find("!dbg") == std::string::npos) {
                int locId = createLocation(currentLineNum, 4, currentSubprogId);
                int locNodeId = 100 + locId;
                line += ", !dbg !" + std::to_string(locNodeId);
            }
        } else if (line == "}") {
            currentSubprogId = -1;
        }

        body << line << "\n";
        currentLineNum++;
    }

    // Assemble final IR: header first, then metadata prolog, body, and epilog
    std::string result = header.str();
    result += generateMetadataProlog();
    result += body.str();
    result += generateMetadataEpilog();

    return result;
}

} // namespace sp::codegen
