#pragma once

// Phase 4 slice C (pro-4.0.0): DebugSession is now a thin DAP-facing layer
// over the real Interpreter (tools/debugger/Interpreter.h). The old
// implementation was a line-walking simulator: it never executed the
// program, it regex-guessed variable values out of the source text (so
// `let z = x + y;` showed the value "x + y"), loops ran once, function
// calls never bound parameters, breakpoints were always "verified" even
// for lines that can never execute, and launch ran the program to
// completion before any breakpoint could be set.
//
// What this class does now:
//   - loadProgram: runs the Interpreter's front-end pipeline (lex, parse,
//     module load, sema). A missing file / syntax error / sema error is a
//     loud failure (loadProgram returns false, getLoadError() explains).
//   - setBreakpoints: verified against the AST's executable lines, per
//     file. A requested line that is a statement start line is verified
//     as-is; otherwise it moves to the next executable line at/after it
//     (verified, client sees the moved line); when no executable line
//     exists at/after the requested line, verified=false (honest).
//   - launch: loads (if needed) and prepares, but does NOT run the
//     program. Execution starts at configurationDone (startExecution) or
//     at stopOnEntry (pause at the first statement). This is the standard
//     DAP flow: launch -> setBreakpoints -> configurationDone -> continue.
//   - Scopes/variables read the interpreter's pause snapshot (real
//     values). Arrays/maps get a variablesReference for expansion.
//   - evaluate runs a real expression in a paused frame; setVariable
//     records a replay override (applied when the program resumes).

#include "DapProtocol.h"
#include "Interpreter.h"
#include <string>
#include <vector>
#include <map>

namespace sp::dap {

class DebugSession {
public:
    DebugSession();

    bool loadProgram(const std::string& path, const std::string& source = "");
    std::vector<Breakpoint> setBreakpoints(const std::string& sourcePath, const std::vector<int>& requestedLines);

    // Launch: load (if needed) and prepare. Does NOT run the program
    // unless stopOnEntry. Execution starts at startExecution()
    // (configurationDone) or at start().
    bool launch(const std::string& path, bool stopOnEntry, bool noDebug);
    // Compat helper used by the tests: launch + run to the first pause.
    bool start(bool stopOnEntry = false);
    // Run from the launch point (configurationDone). No-op when paused
    // or terminated.
    bool startExecution();
    bool stepOver();
    bool stepIn();
    bool stepOut();
    bool continueExecution();

    [[nodiscard]] bool isPaused() const { return m_paused; }
    [[nodiscard]] bool isTerminated() const { return m_terminated; }
    [[nodiscard]] int getCurrentLine() const { return m_currentLine; }
    [[nodiscard]] std::string getPauseReason() const { return m_pauseReason; }
    [[nodiscard]] std::string getLoadError() const { return m_loadError; }
    [[nodiscard]] std::string getLastError() const { return m_lastError; }
    [[nodiscard]] std::string getProgramPath() const { return m_programPath; }

    std::vector<std::string> drainOutput();

    std::vector<Thread> getThreads() const;
    std::vector<StackFrame> getStackFrames() const;
    std::vector<Scope> getScopes(int frameId) const;
    std::vector<Variable> getVariables(int variablesReference) const;
    // Evaluate in a paused frame. Returns false + error when the
    // expression cannot be evaluated (never echoes the expression back
    // as its own value).
    bool evaluate(int frameId, const std::string& expression, Variable& out, std::string& error);

    // Returns false when the frame/variable does not exist or the value
    // is not a literal.
    bool setVariable(int variablesReference, const std::string& name, const std::string& value);

private:
    Interpreter m_interp;
    std::string m_programPath;
    std::string m_loadError;
    std::string m_lastError;
    bool m_loaded{false};
    bool m_launched{false};
    bool m_paused{false};
    bool m_terminated{false};
    int m_currentLine{1};
    std::string m_pauseReason;

    int m_nextBpId{1};
    std::map<std::string, std::vector<Breakpoint>> m_breakpoints;

    // Expanded-value registry: variablesReference -> value, for array/map
    // children. Cleared on every new pause (refs are per-pause). Mutable
    // because getVariables() is const but allocates refs on demand.
    mutable int m_nextExpandedRef{100000};
    mutable std::map<int, InterpValue> m_expandedValues;
    // frameId -> call chain ("main", "main>add"), rebuilt on every pause.
    std::map<int, std::string> m_frameSignatures;

    // Normalize a path the way the interpreter sees it (absolute, no
    // "." / ".." segments) so load/setBreakpoints/stack frames agree.
    static std::string normalizePath(const std::string& path);

    void syncFromInterpreter(const RunOutcome& outcome);
    Variable toVariable(const std::string& name, const InterpValue& value) const;
    const InterpFrame* findPausedFrame(int frameId) const;
    std::string frameChainSignature(int frameId) const;
};

} // namespace sp::dap
