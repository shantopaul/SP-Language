// Phase 4 slice C (pro-4.0.0): DebugSession over the real Interpreter.
// See DebugSession.h for what changed and why.

#include "DebugSession.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace sp::dap {

namespace {

// Scope encoding: frameId * 1000 + 1 = Locals, + 2 = Globals. Expanded
// values (array/map children) use references >= 100000.
constexpr int kScopeStride = 1000;
constexpr int kLocalsScope = 1;
constexpr int kGlobalsScope = 2;
constexpr int kExpandedBase = 100000;

std::string baseName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

} // namespace

DebugSession::DebugSession() = default;

std::string DebugSession::normalizePath(const std::string& path) {
    if (path.empty()) return path;
    std::error_code ec;
    std::filesystem::path abs = std::filesystem::absolute(path, ec);
    if (ec) return path;
    return abs.lexically_normal().string();
}

bool DebugSession::loadProgram(const std::string& path, const std::string& source) {
    m_programPath = normalizePath(path);
    m_loaded = false;
    m_launched = false;
    m_paused = false;
    m_terminated = false;
    m_currentLine = 1;
    m_pauseReason.clear();
    m_loadError.clear();
    m_lastError.clear();
    m_breakpoints.clear();
    m_expandedValues.clear();
    m_frameSignatures.clear();

    std::string error;
    if (!m_interp.load(m_programPath, source, error)) {
        m_loadError = error;
        return false;
    }
    m_loaded = true;
    return true;
}

std::vector<Breakpoint> DebugSession::setBreakpoints(const std::string& sourcePath,
                                                     const std::vector<int>& requestedLines) {
    std::string path = normalizePath(sourcePath);
    std::vector<Breakpoint> result;

    // Breakpoints may arrive before launch; load the program from the
    // breakpoint file so they can be verified. When the file cannot be
    // loaded, every breakpoint is honestly unverified.
    if (!m_loaded) {
        if (!loadProgram(path)) {
            for (int line : requestedLines) {
                Breakpoint bp;
                bp.id = m_nextBpId++;
                bp.verified = false;
                bp.line = line;
                bp.source = Source{baseName(path), path};
                result.push_back(bp);
            }
            return result;
        }
    }

    const std::vector<int>& executable = m_interp.executableLinesFor(path);
    std::vector<int> activeLines;
    for (int line : requestedLines) {
        Breakpoint bp;
        bp.id = m_nextBpId++;
        bp.source = Source{baseName(path), path};
        auto it = std::lower_bound(executable.begin(), executable.end(), line);
        if (it != executable.end() && *it == line) {
            bp.verified = true;
            bp.line = line;
        } else if (it != executable.end()) {
            // Move to the next executable line (standard adapter
            // behavior); the client sees the line that will actually stop.
            bp.verified = true;
            bp.line = *it;
        } else {
            // No executable line at or after the requested line: it can
            // never stop. Say so instead of a silent verified=true.
            bp.verified = false;
            bp.line = line;
        }
        if (bp.verified) activeLines.push_back(bp.line);
        result.push_back(bp);
    }

    m_breakpoints[path] = result;
    m_interp.setBreakpoints(path, activeLines);
    return result;
}

bool DebugSession::launch(const std::string& path, bool stopOnEntry, bool noDebug) {
    if (!m_loaded || normalizePath(path) != m_programPath) {
        if (!loadProgram(path)) return false;
    }
    m_launched = true;
    m_interp.setBreakpointsEnabled(!noDebug);
    if (stopOnEntry) {
        RunOutcome outcome = m_interp.run(RunMode::PauseAtFirst);
        syncFromInterpreter(outcome);
    }
    return true;
}

bool DebugSession::start(bool stopOnEntry) {
    if (!m_loaded) {
        m_lastError = "No program loaded";
        return false;
    }
    if (!m_launched) {
        if (!launch(m_programPath, stopOnEntry, false)) return false;
        if (stopOnEntry) return true; // already paused at the first statement
    }
    return startExecution();
}

bool DebugSession::startExecution() {
    if (m_terminated) {
        m_lastError = "Program has terminated";
        return false;
    }
    if (m_paused) return true; // already paused (e.g. stopOnEntry)
    RunOutcome outcome = m_interp.run(RunMode::Continue);
    syncFromInterpreter(outcome);
    return true;
}

bool DebugSession::continueExecution() {
    if (m_terminated) {
        m_lastError = "Program has terminated";
        return false;
    }
    RunOutcome outcome = m_interp.run(RunMode::Continue);
    syncFromInterpreter(outcome);
    return true;
}

bool DebugSession::stepOver() {
    if (m_terminated) {
        m_lastError = "Program has terminated";
        return false;
    }
    RunOutcome outcome = m_interp.run(RunMode::StepOver);
    syncFromInterpreter(outcome);
    return true;
}

bool DebugSession::stepIn() {
    if (m_terminated) {
        m_lastError = "Program has terminated";
        return false;
    }
    RunOutcome outcome = m_interp.run(RunMode::StepIn);
    syncFromInterpreter(outcome);
    return true;
}

bool DebugSession::stepOut() {
    if (m_terminated) {
        m_lastError = "Program has terminated";
        return false;
    }
    RunOutcome outcome = m_interp.run(RunMode::StepOut);
    syncFromInterpreter(outcome);
    return true;
}

void DebugSession::syncFromInterpreter(const RunOutcome& outcome) {
    m_paused = outcome.paused;
    m_terminated = outcome.terminated;
    m_lastError = outcome.error;
    m_expandedValues.clear();
    m_frameSignatures.clear();

    if (outcome.paused) {
        const auto& frames = m_interp.pausedFrames();
        if (!frames.empty()) m_currentLine = frames.front().line;
        m_pauseReason = m_interp.pauseReason();
        // Rebuild frameId -> call-chain signatures (outermost first).
        std::string chain;
        for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
            if (!chain.empty()) chain += ">";
            chain += it->name;
            m_frameSignatures[it->id] = chain;
        }
    } else if (outcome.terminated) {
        // The pause snapshot is stale once the program ends: a terminated
        // session has no stack, no variables, and nothing to evaluate.
        // (Without this, evaluate/variables would silently answer from
        // the last pause point after the program is gone.)
        m_interp.clearPausedSnapshot();
        if (!outcome.error.empty()) m_currentLine = outcome.errorLine;
    }
}

std::vector<std::string> DebugSession::drainOutput() {
    return m_interp.drainOutput();
}

std::vector<Thread> DebugSession::getThreads() const {
    return {Thread{1, "main"}};
}

std::vector<StackFrame> DebugSession::getStackFrames() const {
    std::vector<StackFrame> frames;
    for (const auto& f : m_interp.pausedFrames()) {
        StackFrame sf;
        sf.id = f.id;
        sf.name = f.name;
        std::string file = f.file.empty() ? m_programPath : f.file;
        sf.source = Source{baseName(file), file};
        sf.line = f.line;
        sf.column = f.column;
        frames.push_back(sf);
    }
    return frames;
}

const InterpFrame* DebugSession::findPausedFrame(int frameId) const {
    for (const auto& f : m_interp.pausedFrames()) {
        if (f.id == frameId) return &f;
    }
    return nullptr;
}

std::string DebugSession::frameChainSignature(int frameId) const {
    auto it = m_frameSignatures.find(frameId);
    return it == m_frameSignatures.end() ? std::string() : it->second;
}

std::vector<Scope> DebugSession::getScopes(int frameId) const {
    if (!findPausedFrame(frameId)) return {};
    return {Scope{"Locals", frameId * kScopeStride + kLocalsScope, false},
            Scope{"Globals", frameId * kScopeStride + kGlobalsScope, false}};
}

Variable DebugSession::toVariable(const std::string& name, const InterpValue& value) const {
    Variable v;
    v.name = name;
    v.value = value.display();
    v.type = value.typeName();
    v.variablesReference = 0;
    if ((value.kind == InterpValue::Kind::Array && !value.arrayVal.empty()) ||
        (value.kind == InterpValue::Kind::Map && !value.mapVal.empty())) {
        int ref = m_nextExpandedRef++;
        m_expandedValues[ref] = value;
        v.variablesReference = ref;
    }
    return v;
}

std::vector<Variable> DebugSession::getVariables(int variablesReference) const {
    // Expanded array/map children.
    if (variablesReference >= kExpandedBase) {
        auto it = m_expandedValues.find(variablesReference);
        if (it == m_expandedValues.end()) return {};
        std::vector<Variable> children;
        const InterpValue& value = it->second;
        if (value.kind == InterpValue::Kind::Array) {
            for (size_t i = 0; i < value.arrayVal.size(); ++i) {
                children.push_back(toVariable("[" + std::to_string(i) + "]", value.arrayVal[i]));
            }
        } else if (value.kind == InterpValue::Kind::Map) {
            for (const auto& [key, val] : value.mapVal) {
                children.push_back(toVariable(key, val));
            }
        }
        return children;
    }

    int frameId = variablesReference / kScopeStride;
    int kind = variablesReference % kScopeStride;
    const InterpFrame* frame = findPausedFrame(frameId);
    if (!frame) return {};

    std::vector<Variable> result;
    if (kind == kLocalsScope) {
        // Innermost scope first; a name shadowed in an inner scope wins.
        std::map<std::string, bool> seen;
        for (auto it = frame->scopes.rbegin(); it != frame->scopes.rend(); ++it) {
            for (const auto& [name, value] : *it) {
                if (seen[name]) continue;
                seen[name] = true;
                result.push_back(toVariable(name, value));
            }
        }
        std::sort(result.begin(), result.end(),
                  [](const Variable& a, const Variable& b) { return a.name < b.name; });
    } else if (kind == kGlobalsScope) {
        for (const auto& [name, value] : m_interp.pausedGlobals()) {
            result.push_back(toVariable(name, value));
        }
        std::sort(result.begin(), result.end(),
                  [](const Variable& a, const Variable& b) { return a.name < b.name; });
    }
    return result;
}

bool DebugSession::evaluate(int frameId, const std::string& expression, Variable& out,
                            std::string& error) {
    if (m_terminated) {
        error = "Program has terminated";
        return false;
    }
    InterpValue value;
    if (!m_interp.evaluateInFrame(frameId, expression, value, error)) return false;
    out = toVariable(expression, value);
    return true;
}

bool DebugSession::setVariable(int variablesReference, const std::string& name,
                               const std::string& value) {
    int frameId = variablesReference / kScopeStride;
    int kind = variablesReference % kScopeStride;
    if (kind != kLocalsScope && kind != kGlobalsScope) return false;
    std::string signature = frameChainSignature(frameId);
    if (signature.empty()) return false;

    InterpValue parsed;
    if (!Interpreter::parseLiteralValue(value, parsed)) return false;
    if (kind == kGlobalsScope) {
        // Globals live in the interpreter's module-level map, not in any
        // frame's scope.
        return m_interp.setVariableOverride("<globals>", name, parsed);
    }
    return m_interp.setVariableOverride(signature, name, parsed);
}

} // namespace sp::dap
