#include "DebugSession.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace sp::dap {

namespace {

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

} // namespace

DebugSession::DebugSession() = default;

bool DebugSession::loadProgram(const std::string& path, const std::string& source) {
    m_programPath = path;
    m_source = source;

    if (m_source.empty()) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        std::ostringstream ss;
        ss << file.rdbuf();
        m_source = ss.str();
    }

    m_lines.clear();
    std::istringstream stream(m_source);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        m_lines.push_back(line);
    }

    parseExecutableLines();
    m_currentLineIndex = 0;
    m_currentLine = m_executableLines.empty() ? 1 : m_executableLines[0];
    m_paused = false;
    m_terminated = false;
    m_callStack.clear();
    m_frameVariables.clear();
    m_globalVariables.clear();

    return true;
}

void DebugSession::parseExecutableLines() {
    m_executableLines.clear();
    for (size_t i = 0; i < m_lines.size(); ++i) {
        std::string trimmed = trim(m_lines[i]);
        if (trimmed.empty()) continue;
        if (trimmed.starts_with("//") || trimmed.starts_with("/*") || trimmed.starts_with("*")) continue;
        if (trimmed == "{" || trimmed == "}" || trimmed == "};") continue;
        m_executableLines.push_back(static_cast<int>(i + 1));
    }
}

std::vector<Breakpoint> DebugSession::setBreakpoints(const std::string& sourcePath, const std::vector<int>& requestedLines) {
    std::vector<Breakpoint> result;
    for (int reqLine : requestedLines) {
        int bestLine = reqLine;
        // Find closest executable line
        for (int exLine : m_executableLines) {
            if (exLine >= reqLine) {
                bestLine = exLine;
                break;
            }
        }

        Breakpoint bp;
        bp.id = m_nextBpId++;
        bp.verified = true;
        bp.line = bestLine;
        bp.source.path = sourcePath;
        bp.source.name = sourcePath;
        result.push_back(bp);
    }

    m_breakpoints[sourcePath] = result;
    return result;
}

bool DebugSession::hasBreakpointAt(const std::string& path, int line) const {
    auto it = m_breakpoints.find(path);
    if (it != m_breakpoints.end()) {
        for (const auto& bp : it->second) {
            if (bp.line == line) return true;
        }
    }
    // Also check wildcard or empty path
    for (const auto& [p, bps] : m_breakpoints) {
        for (const auto& bp : bps) {
            if (bp.line == line) return true;
        }
    }
    return false;
}

bool DebugSession::start(bool stopOnEntry) {
    if (m_executableLines.empty()) {
        m_terminated = true;
        return false;
    }

    m_currentLineIndex = 0;
    m_currentLine = m_executableLines[0];
    m_terminated = false;

    m_callStack.clear();
    StackFrame rootFrame;
    rootFrame.id = 1;
    rootFrame.name = "main";
    rootFrame.source.path = m_programPath;
    rootFrame.source.name = "main.sp";
    rootFrame.line = m_currentLine;
    rootFrame.column = 1;
    m_callStack.push_back(rootFrame);

    updateVariableStateForLine(m_currentLine);

    if (stopOnEntry) {
        m_paused = true;
        m_pauseReason = "entry";
        return true;
    }

    return continueExecution();
}

void DebugSession::updateVariableStateForLine(int line) {
    if (line <= 0 || static_cast<size_t>(line) > m_lines.size()) return;
    std::string text = trim(m_lines[line - 1]);

    int frameId = m_callStack.empty() ? 1 : m_callStack.front().id;

    // Detect variable declaration: let x = 10; or const y = "abc";
    std::regex varDeclRegex(R"((let|const)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(:\s*[a-zA-Z_][a-zA-Z0-9_]*)?\s*=\s*([^;]+);)");
    std::smatch match;
    if (std::regex_search(text, match, varDeclRegex)) {
        std::string varName = match[2];
        std::string valStr = trim(match[4]);
        std::string type = "int";

        if (valStr.starts_with("\"") && valStr.ends_with("\"")) {
            type = "string";
        } else if (valStr == "true" || valStr == "false") {
            type = "bool";
        } else if (valStr.find('.') != std::string::npos) {
            type = "float";
        }

        Variable v;
        v.name = varName;
        v.value = valStr;
        v.type = type;
        v.variablesReference = 0;
        m_frameVariables[frameId][varName] = v;
    }

    // Detect assignment: x = 20;
    std::regex assignRegex(R"(^([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*([^;]+);)");
    if (std::regex_search(text, match, assignRegex)) {
        std::string varName = match[1];
        std::string valStr = trim(match[2]);
        auto it = m_frameVariables[frameId].find(varName);
        if (it != m_frameVariables[frameId].end()) {
            it->second.value = valStr;
        }
    }
}

bool DebugSession::continueExecution() {
    if (m_terminated) return false;

    while (m_currentLineIndex + 1 < m_executableLines.size()) {
        m_currentLineIndex++;
        m_currentLine = m_executableLines[m_currentLineIndex];
        if (!m_callStack.empty()) {
            m_callStack.front().line = m_currentLine;
        }
        updateVariableStateForLine(m_currentLine);

        if (hasBreakpointAt(m_programPath, m_currentLine)) {
            m_paused = true;
            m_pauseReason = "breakpoint";
            return true;
        }
    }

    m_paused = false;
    m_terminated = true;
    return true;
}

bool DebugSession::stepOver() {
    if (m_terminated) return false;

    if (m_currentLineIndex + 1 < m_executableLines.size()) {
        m_currentLineIndex++;
        m_currentLine = m_executableLines[m_currentLineIndex];
        if (!m_callStack.empty()) {
            m_callStack.front().line = m_currentLine;
        }
        updateVariableStateForLine(m_currentLine);
        m_paused = true;
        m_pauseReason = "step";
        return true;
    }

    m_paused = false;
    m_terminated = true;
    return true;
}

bool DebugSession::stepIn() {
    if (m_terminated) return false;

    if (static_cast<size_t>(m_currentLine) <= m_lines.size()) {
        std::string curText = trim(m_lines[m_currentLine - 1]);
        // If current line calls a function like foo()
        std::regex callRegex(R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*\()");
        std::smatch match;
        if (std::regex_search(curText, match, callRegex)) {
            std::string callee = match[1];
            if (callee != "println" && callee != "print" && callee != "assert") {
                // Find function definition in lines
                for (size_t i = 0; i < m_lines.size(); ++i) {
                    if (m_lines[i].find("fn " + callee) != std::string::npos) {
                        StackFrame frame;
                        frame.id = static_cast<int>(m_callStack.size() + 1);
                        frame.name = callee;
                        frame.source.path = m_programPath;
                        frame.source.name = "main.sp";
                        frame.line = static_cast<int>(i + 1);
                        frame.column = 1;
                        m_callStack.insert(m_callStack.begin(), frame);

                        m_currentLine = frame.line;
                        for (size_t k = 0; k < m_executableLines.size(); ++k) {
                            if (m_executableLines[k] >= m_currentLine) {
                                m_currentLineIndex = k;
                                break;
                            }
                        }
                        updateVariableStateForLine(m_currentLine);
                        m_paused = true;
                        m_pauseReason = "step";
                        return true;
                    }
                }
            }
        }
    }

    return stepOver();
}

bool DebugSession::stepOut() {
    if (m_terminated) return false;

    if (m_callStack.size() > 1) {
        m_callStack.erase(m_callStack.begin());
    }
    return stepOver();
}

std::vector<Thread> DebugSession::getThreads() const {
    return { Thread{ 1, "main" } };
}

std::vector<StackFrame> DebugSession::getStackFrames() const {
    return m_callStack;
}

std::vector<Scope> DebugSession::getScopes(int frameId) const {
    std::vector<Scope> scopes;
    scopes.push_back(Scope{ "Locals", frameId * 1000 + 1, false });
    scopes.push_back(Scope{ "Globals", frameId * 1000 + 2, false });
    return scopes;
}

std::vector<Variable> DebugSession::getVariables(int variablesReference) const {
    std::vector<Variable> vars;
    int frameId = variablesReference / 1000;
    int scopeType = variablesReference % 1000;

    if (scopeType == 1) { // Locals
        auto it = m_frameVariables.find(frameId);
        if (it != m_frameVariables.end()) {
            for (const auto& [name, v] : it->second) {
                vars.push_back(v);
            }
        }
    } else if (scopeType == 2) { // Globals
        for (const auto& [name, v] : m_globalVariables) {
            vars.push_back(v);
        }
    }

    return vars;
}

Variable DebugSession::evaluate(int frameId, const std::string& expression) const {
    std::string expr = trim(expression);

    auto it = m_frameVariables.find(frameId);
    if (it != m_frameVariables.end()) {
        auto varIt = it->second.find(expr);
        if (varIt != it->second.end()) {
            return varIt->second;
        }
    }

    // Try global
    auto gIt = m_globalVariables.find(expr);
    if (gIt != m_globalVariables.end()) {
        return gIt->second;
    }

    // Literal check
    Variable lit;
    lit.name = expr;
    lit.value = expr;
    lit.type = "any";
    return lit;
}

void DebugSession::setVariable(int variablesReference, const std::string& name, const std::string& value) {
    int frameId = variablesReference / 1000;
    m_frameVariables[frameId][name].value = value;
}

} // namespace sp::dap
