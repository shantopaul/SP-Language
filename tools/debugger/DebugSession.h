#pragma once

#include "DapProtocol.h"
#include <string>
#include <vector>
#include <map>

namespace sp::dap {

class DebugSession {
public:
    DebugSession();

    bool loadProgram(const std::string& path, const std::string& source = "");
    std::vector<Breakpoint> setBreakpoints(const std::string& sourcePath, const std::vector<int>& requestedLines);

    bool start(bool stopOnEntry = false);
    bool stepOver();
    bool stepIn();
    bool stepOut();
    bool continueExecution();

    [[nodiscard]] bool isPaused() const { return m_paused; }
    [[nodiscard]] bool isTerminated() const { return m_terminated; }
    [[nodiscard]] int getCurrentLine() const { return m_currentLine; }
    [[nodiscard]] std::string getPauseReason() const { return m_pauseReason; }

    std::vector<Thread> getThreads() const;
    std::vector<StackFrame> getStackFrames() const;
    std::vector<Scope> getScopes(int frameId) const;
    std::vector<Variable> getVariables(int variablesReference) const;
    Variable evaluate(int frameId, const std::string& expression) const;

    void setVariable(int variablesReference, const std::string& name, const std::string& value);

private:
    std::string m_programPath;
    std::string m_source;
    std::vector<std::string> m_lines;
    std::vector<int> m_executableLines;
    size_t m_currentLineIndex{0};
    int m_currentLine{1};
    bool m_paused{false};
    bool m_terminated{false};
    std::string m_pauseReason;

    int m_nextBpId{1};
    std::map<std::string, std::vector<Breakpoint>> m_breakpoints;

    std::vector<StackFrame> m_callStack;
    std::map<int, std::map<std::string, Variable>> m_frameVariables;
    std::map<std::string, Variable> m_globalVariables;

    void parseExecutableLines();
    void updateVariableStateForLine(int line);
    bool hasBreakpointAt(const std::string& path, int line) const;
};

} // namespace sp::dap
