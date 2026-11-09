// Phase 4 slice C (pro-4.0.0): tree-walking interpreter for the debugger.
// See Interpreter.h for the design (real values, real loops, real calls,
// replay-based pause/resume). Anything this engine cannot do raises a loud
// InterpError instead of producing a silent wrong value.

#include "Interpreter.h"

#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/lexer/Lexer.h"
#include "sp/lexer/TokenType.h"
#include "sp/parser/Parser.h"
#include "sp/sema/ComptimeEvaluator.h"
#include "sp/sema/ModuleLoader.h"
#include "sp/sema/SemanticAnalyzer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace sp::dap {

namespace {

// Safety valves: a debuggee that never terminates must not hang the
// adapter forever, and unbounded recursion must not overflow the C++
// stack (the interpreter recurses per SP call frame).
constexpr int kMaxSteps = 1000000;
constexpr int kMaxCallDepth = 1000;

bool isNumber(const InterpValue& v) {
    return v.kind == InterpValue::Kind::Int || v.kind == InterpValue::Kind::Float;
}

double asDouble(const InterpValue& v) {
    return v.kind == InterpValue::Kind::Float ? v.floatVal : static_cast<double>(v.intVal);
}

// Deep equality for == / != (numbers compare across int/float).
bool valuesEqual(const InterpValue& a, const InterpValue& b) {
    if (isNumber(a) && isNumber(b)) return asDouble(a) == asDouble(b);
    if (a.kind != b.kind) return false;
    switch (a.kind) {
        case InterpValue::Kind::String: return a.stringVal == b.stringVal;
        case InterpValue::Kind::Char: return a.charVal == b.charVal;
        case InterpValue::Kind::Bool: return a.boolVal == b.boolVal;
        case InterpValue::Kind::Null: return true;
        case InterpValue::Kind::Range: return a.intVal == b.intVal && a.rangeEnd == b.rangeEnd;
        case InterpValue::Kind::Array: {
            if (a.arrayVal.size() != b.arrayVal.size()) return false;
            for (size_t i = 0; i < a.arrayVal.size(); ++i) {
                if (!valuesEqual(a.arrayVal[i], b.arrayVal[i])) return false;
            }
            return true;
        }
        case InterpValue::Kind::Map: {
            if (a.mapVal.size() != b.mapVal.size()) return false;
            for (const auto& [k, v] : a.mapVal) {
                auto it = b.mapVal.find(k);
                if (it == b.mapVal.end() || !valuesEqual(v, it->second)) return false;
            }
            return true;
        }
        default: return false;
    }
}

std::string escapeForDisplay(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            default: out += c; break;
        }
    }
    return out;
}

// RAII scope guard for InterpFrame scopes (pops even when a control-flow
// signal unwinds through the block).
struct ScopeGuard {
    InterpFrame& frame;
    explicit ScopeGuard(InterpFrame& f) : frame(f) { frame.scopes.emplace_back(); }
    ~ScopeGuard() {
        if (!frame.scopes.empty()) frame.scopes.pop_back();
    }
};

// RAII frame guard: pops the callee frame even when an exception
// unwinds through the call.
struct FrameGuard {
    std::vector<InterpFrame>& frames;
    explicit FrameGuard(std::vector<InterpFrame>& f) : frames(f) {}
    ~FrameGuard() {
        if (!frames.empty()) frames.pop_back();
    }
};

std::string formatDiagnostics(const diagnostics::DiagnosticEngine& diags, size_t maxShown = 3) {
    std::ostringstream out;
    size_t shown = 0;
    for (const auto& d : diags.getDiagnostics()) {
        if (d.severity != diagnostics::DiagnosticSeverity::Error) continue;
        if (shown > 0) out << "; ";
        out << d.code << ": " << d.message;
        if (!d.location.filename.empty()) {
            out << " (" << d.location.filename << ":" << d.location.line << ":" << d.location.column << ")";
        }
        if (++shown >= maxShown) break;
    }
    if (shown == 0) out << "unknown error";
    return out.str();
}

} // namespace

// ---------------------------------------------------------------------------
// InterpValue
// ---------------------------------------------------------------------------

InterpValue InterpValue::fromInt(int64_t v) {
    InterpValue r;
    r.kind = Kind::Int;
    r.intVal = v;
    return r;
}
InterpValue InterpValue::fromFloat(double v) {
    InterpValue r;
    r.kind = Kind::Float;
    r.floatVal = v;
    return r;
}
InterpValue InterpValue::fromBool(bool v) {
    InterpValue r;
    r.kind = Kind::Bool;
    r.boolVal = v;
    return r;
}
InterpValue InterpValue::fromString(std::string v) {
    InterpValue r;
    r.kind = Kind::String;
    r.stringVal = std::move(v);
    return r;
}
InterpValue InterpValue::fromChar(char v) {
    InterpValue r;
    r.kind = Kind::Char;
    r.charVal = v;
    return r;
}
InterpValue InterpValue::fromArray(std::vector<InterpValue> v) {
    InterpValue r;
    r.kind = Kind::Array;
    r.arrayVal = std::move(v);
    return r;
}
InterpValue InterpValue::fromMap(std::map<std::string, InterpValue> v) {
    InterpValue r;
    r.kind = Kind::Map;
    r.mapVal = std::move(v);
    return r;
}
InterpValue InterpValue::fromRange(int64_t start, int64_t end) {
    InterpValue r;
    r.kind = Kind::Range;
    r.intVal = start;
    r.rangeEnd = end;
    return r;
}
InterpValue InterpValue::nullVal() {
    InterpValue r;
    r.kind = Kind::Null;
    return r;
}
InterpValue InterpValue::voidVal() {
    InterpValue r;
    r.kind = Kind::Void;
    return r;
}

std::string InterpValue::display() const {
    std::ostringstream out;
    switch (kind) {
        case Kind::Void: return "void";
        case Kind::Null: return "null";
        case Kind::Int: return std::to_string(intVal);
        case Kind::Float: {
            out << floatVal;
            return out.str();
        }
        case Kind::Bool: return boolVal ? "true" : "false";
        case Kind::String: return "\"" + escapeForDisplay(stringVal) + "\"";
        case Kind::Char: {
            std::string s(1, charVal);
            return "'" + escapeForDisplay(s) + "'";
        }
        case Kind::Range: return std::to_string(intVal) + ".." + std::to_string(rangeEnd);
        case Kind::Array: {
            out << "[";
            for (size_t i = 0; i < arrayVal.size(); ++i) {
                if (i > 0) out << ", ";
                out << arrayVal[i].display();
            }
            out << "]";
            return out.str();
        }
        case Kind::Map: {
            out << "{";
            bool first = true;
            for (const auto& [k, v] : mapVal) {
                if (!first) out << ", ";
                first = false;
                out << "\"" << escapeForDisplay(k) << "\": " << v.display();
            }
            out << "}";
            return out.str();
        }
    }
    return "";
}

std::string InterpValue::typeName() const {
    switch (kind) {
        case Kind::Void: return "void";
        case Kind::Null: return "null";
        case Kind::Int: return "int";
        case Kind::Float: return "float";
        case Kind::Bool: return "bool";
        case Kind::String: return "string";
        case Kind::Char: return "char";
        case Kind::Range: return "range";
        case Kind::Array: {
            if (arrayVal.empty()) return "array";
            return arrayVal[0].typeName() + "[]";
        }
        case Kind::Map: {
            if (mapVal.empty()) return "map";
            return mapVal.begin()->second.typeName() + "{}";
        }
    }
    return "";
}

bool InterpValue::isTruthy() const {
    switch (kind) {
        case Kind::Bool: return boolVal;
        case Kind::Int: return intVal != 0;
        case Kind::Float: return floatVal != 0.0;
        case Kind::String: return !stringVal.empty();
        case Kind::Char: return charVal != '\0';
        case Kind::Array: return !arrayVal.empty();
        case Kind::Map: return !mapVal.empty();
        case Kind::Null: return false;
        case Kind::Void: return false;
        case Kind::Range: return intVal <= rangeEnd;
    }
    return false;
}

std::string InterpValue::toRawString() const {
    switch (kind) {
        case Kind::String: return stringVal;
        case Kind::Char: return std::string(1, charVal);
        case Kind::Int: return std::to_string(intVal);
        case Kind::Float: {
            std::ostringstream out;
            out << floatVal;
            return out.str();
        }
        case Kind::Bool: return boolVal ? "true" : "false";
        case Kind::Null: return "null";
        case Kind::Void: return "";
        case Kind::Range: return std::to_string(intVal) + ".." + std::to_string(rangeEnd);
        case Kind::Array:
        case Kind::Map: return display();
    }
    return "";
}

// ---------------------------------------------------------------------------
// InterpFrame
// ---------------------------------------------------------------------------

std::string InterpFrame::signature() const {
    // A stable key for a frame across replays: the call chain by name.
    // DebugSession builds it from the paused snapshot; the interpreter
    // uses it to match setVariable overrides on resume.
    return name;
}

bool InterpFrame::lookup(const std::string& name, InterpValue& out) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            out = found->second;
            return true;
        }
    }
    return false;
}

bool InterpFrame::assign(const std::string& name, const InterpValue& value) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            found->second = value;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Load: the same front-end pipeline `spc` uses.
// ---------------------------------------------------------------------------

bool Interpreter::load(const std::string& path, const std::string& source, std::string& error) {
    // A fresh load is a fresh execution: reset all run state (a previous
    // program may have terminated or paused in this interpreter; without
    // this, launching a second program in the same session would return
    // the first program's terminal state at once).
    m_programPath = path;
    m_program.reset();
    m_functions.clear();
    m_topLevelStatements.clear();
    m_mainFn = nullptr;
    m_executableLinesByFile.clear();
    m_frames.clear();
    m_globals.clear();
    m_output.clear();
    m_outputPending.clear();
    m_breakpointsByFile.clear();
    m_breakpointsEnabled = true;
    m_paused = false;
    m_terminated = false;
    m_started = false;
    m_pauseReason.clear();
    m_pauseSerial = -1;
    m_pauseDepth = 0;
    m_skipSerial = -1;
    m_stmtSerial = 0;
    m_steps = 0;
    m_replaying = false;
    m_replayTarget = -1;
    m_suppressOutput = false;
    m_evaluating = false;
    m_pausedFrames.clear();
    m_pausedGlobals.clear();
    m_overrides.clear();

    std::string src = source;
    if (src.empty()) {
        std::ifstream in(path);
        if (!in) {
            error = "Cannot open program file '" + path + "'";
            return false;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        src = ss.str();
    }

    std::ostringstream sink; // diagnostics are read back via getDiagnostics()
    diagnostics::DiagnosticEngine diags(sink);

    lexer::Lexer lexer(src, path, &diags);
    std::vector<lexer::Token> tokens = lexer.tokenizeAll();
    if (diags.hasErrors()) {
        error = "Lex error: " + formatDiagnostics(diags);
        return false;
    }

    parser::Parser parser(tokens, path, &diags);
    auto program = parser.parseProgram();
    if (!program || diags.hasErrors()) {
        error = "Parse error: " + formatDiagnostics(diags);
        return false;
    }

    sema::ModuleLoader loader(diags);
    std::filesystem::path programPath(path);
    if (programPath.has_parent_path()) loader.addSearchPath(programPath.parent_path());
    if (!loader.loadAndMergeAll(*program, programPath)) {
        error = "Module load error: " + formatDiagnostics(diags);
        return false;
    }

    sema::ComptimeEvaluator comptime(&diags);
    comptime.processProgram(*program);
    if (diags.hasErrors()) {
        error = "Comptime error: " + formatDiagnostics(diags);
        return false;
    }

    sema::SemanticAnalyzer sema(diags);
    if (!sema.analyze(*program) || diags.hasErrors()) {
        error = "Semantic error: " + formatDiagnostics(diags);
        return false;
    }

    m_program = std::move(program);
    m_functions.clear();
    m_topLevelStatements.clear();
    m_mainFn = nullptr;
    for (const auto& decl : m_program->getDeclarations()) {
        if (!decl) continue;
        if (decl->getKind() == ast::ASTNodeKind::FunctionDecl) {
            const auto* fn = static_cast<const ast::FunctionDecl*>(decl.get());
            m_functions[fn->getName()].push_back(fn);
            if (fn->getName() == "main" && fn->getParams().empty()) m_mainFn = fn;
        } else if (decl->getKind() == ast::ASTNodeKind::ClassDecl ||
                   decl->getKind() == ast::ASTNodeKind::StructDecl ||
                   decl->getKind() == ast::ASTNodeKind::EnumDecl ||
                   decl->getKind() == ast::ASTNodeKind::InterfaceDecl ||
                   decl->getKind() == ast::ASTNodeKind::ImportDecl ||
                   decl->getKind() == ast::ASTNodeKind::ModuleDecl) {
            // Type declarations and imports: no runtime effect in the debugger.
            continue;
        } else if (const auto* stmt = dynamic_cast<const ast::StatementNode*>(decl.get())) {
            m_topLevelStatements.push_back(stmt);
        }
    }

    if (!m_mainFn && m_topLevelStatements.empty()) {
        error = "No 'main' function or top-level statements found in '" + path + "'";
        return false;
    }

    collectExecutableLines();
    return true;
}

void Interpreter::collectExecutableLines() {
    m_executableLinesByFile.clear();
    for (const auto& decl : m_program->getDeclarations()) {
        if (!decl) continue;
        if (decl->getKind() == ast::ASTNodeKind::FunctionDecl) {
            const auto* fn = static_cast<const ast::FunctionDecl*>(decl.get());
            if (fn->hasBody()) collectFromBlock(*fn->getBody());
        } else if (const auto* stmt = dynamic_cast<const ast::StatementNode*>(decl.get())) {
            collectFromStmt(*stmt);
        }
    }
    for (auto& [file, lines] : m_executableLinesByFile) {
        std::sort(lines.begin(), lines.end());
        lines.erase(std::unique(lines.begin(), lines.end()), lines.end());
    }
}

void Interpreter::collectFromBlock(const ast::BlockStmt& block) {
    for (const auto& stmt : block.getStatements()) {
        if (stmt) collectFromStmt(*stmt);
    }
}

void Interpreter::collectFromStmt(const ast::StatementNode& stmt) {
    using Kind = ast::ASTNodeKind;
    // A statement's own start line is breakable (except a bare block,
    // whose line is its opener's line, and function declarations, which
    // are definitions, not executions). Keyed by the statement's file, so
    // breakpoints in one file never fire for a same-numbered line in
    // another file.
    if (stmt.getKind() != Kind::BlockStmt && stmt.getKind() != Kind::FunctionDecl) {
        m_executableLinesByFile[stmt.getLocation().filename].push_back(stmt.getLocation().line);
    }
    switch (stmt.getKind()) {
        case Kind::BlockStmt: {
            collectFromBlock(static_cast<const ast::BlockStmt&>(stmt));
            break;
        }
        case Kind::IfStmt: {
            const auto& ifs = static_cast<const ast::IfStmt&>(stmt);
            collectFromStmt(ifs.getThenBranch());
            if (const auto* els = ifs.getElseBranch()) collectFromStmt(*els);
            break;
        }
        case Kind::WhileStmt: {
            const auto& w = static_cast<const ast::WhileStmt&>(stmt);
            collectFromStmt(w.getBody());
            break;
        }
        case Kind::DoWhileStmt: {
            const auto& w = static_cast<const ast::DoWhileStmt&>(stmt);
            collectFromStmt(w.getBody());
            break;
        }
        case Kind::ForStmt: {
            const auto& f = static_cast<const ast::ForStmt&>(stmt);
            collectFromStmt(f.getBody());
            break;
        }
        case Kind::ForInStmt: {
            const auto& f = static_cast<const ast::ForInStmt&>(stmt);
            collectFromStmt(f.getBody());
            break;
        }
        case Kind::TryCatchStmt: {
            const auto& t = static_cast<const ast::TryCatchStmt&>(stmt);
            collectFromBlock(t.getTryBlock());
            for (const auto& c : t.getCatches()) collectFromBlock(c.getBody());
            if (const auto* fin = t.getFinallyBlock()) collectFromBlock(*fin);
            break;
        }
        case Kind::SwitchStmt: {
            const auto& s = static_cast<const ast::SwitchStmt&>(stmt);
            for (const auto& c : s.getCases()) {
                for (const auto& cs : c.getStatements()) {
                    if (cs) collectFromStmt(*cs);
                }
            }
            for (const auto& ds : s.getDefaultStatements()) {
                if (ds) collectFromStmt(*ds);
            }
            break;
        }
        case Kind::MatchStmt: {
            const auto& m = static_cast<const ast::MatchStmt&>(stmt);
            for (const auto& arm : m.getArms()) {
                if (const auto* s = dynamic_cast<const ast::StatementNode*>(&arm.getBody())) {
                    collectFromStmt(*s);
                }
            }
            break;
        }
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Run / pause / replay
// ---------------------------------------------------------------------------

void Interpreter::emitOutput(const std::string& line) {
    if (m_suppressOutput) return;
    if (!m_outputPending.empty()) {
        m_output.push_back(m_outputPending + line);
        m_outputPending.clear();
    } else {
        m_output.push_back(line);
    }
}

void Interpreter::emitOutputPartial(const std::string& text) {
    if (m_suppressOutput) return;
    m_outputPending += text;
}

[[noreturn]] void Interpreter::fail(const std::string& message, int line) {
    throw InterpError{message, line};
}

std::vector<std::string> Interpreter::drainOutput() {
    if (!m_outputPending.empty()) {
        m_output.push_back(m_outputPending);
        m_outputPending.clear();
    }
    std::vector<std::string> out = std::move(m_output);
    m_output.clear();
    return out;
}

void Interpreter::takePauseSnapshot(int serial, int depth, const std::string& reason) {
    m_pauseReason = reason;
    m_pauseDepth = depth;
    m_pauseSerial = serial;
    m_pausedFrames.clear();
    for (auto it = m_frames.rbegin(); it != m_frames.rend(); ++it) m_pausedFrames.push_back(*it);
    m_pausedGlobals = m_globals;
    m_paused = true;
}

void Interpreter::beforeStatement(const ast::StatementNode& stmt, InterpFrame& frame, int depth,
                                  RunMode mode) {
    const int serial = m_stmtSerial;
    const int line = stmt.getLocation().line;
    frame.line = line;
    frame.column = stmt.getLocation().column;
    frame.file = stmt.getLocation().filename;

    // evaluateInFrame runs code with pauses disabled (a breakpoint inside
    // an evaluated call must not pause the evaluation).
    if (m_evaluating) return;

    if (m_replaying) {
        if (serial < m_replayTarget) return; // fast-forward, no pauses, no output
        // Reached the pause point: stop replaying, re-apply setVariable
        // overrides, then fall through to the mode check below. The paused
        // statement itself executes once without re-triggering its own
        // breakpoint (serial == m_skipSerial).
        m_replaying = false;
        m_suppressOutput = false;
        applyOverridesAtPausePoint();
    }

    // Resuming into the statement we paused before: execute it once.
    if (serial == m_skipSerial) return;

    switch (mode) {
        case RunMode::PauseAtFirst:
            if (serial == 0) takePauseSnapshot(serial, depth, "entry");
            break;
        case RunMode::Continue: {
            if (m_breakpointsEnabled) {
                auto it = m_breakpointsByFile.find(stmt.getLocation().filename);
                if (it != m_breakpointsByFile.end() &&
                    std::find(it->second.begin(), it->second.end(), line) != it->second.end()) {
                    takePauseSnapshot(serial, depth, "breakpoint");
                }
            }
            break;
        }
        case RunMode::StepOver:
            if (depth <= m_pauseDepth) takePauseSnapshot(serial, depth, "step");
            break;
        case RunMode::StepIn:
            takePauseSnapshot(serial, depth, "step");
            break;
        case RunMode::StepOut:
            if (depth < m_pauseDepth) takePauseSnapshot(serial, depth, "step");
            break;
    }
    if (m_paused) throw InterpPause{};
}

RunOutcome Interpreter::run(RunMode mode) {
    RunOutcome outcome;
    if (m_terminated) {
        outcome.terminated = true;
        return outcome;
    }
    if (!m_program) {
        outcome.terminated = true;
        outcome.error = "No program loaded";
        return outcome;
    }

    // Frames are held by reference across nested calls; reserve enough
    // capacity up front so push_back never reallocates (and never
    // invalidates a live InterpFrame&). Depth is capped at kMaxCallDepth.
    m_frames.reserve(static_cast<size_t>(kMaxCallDepth) + 8);
    m_runMode = mode;

    if (!m_started) {
        m_started = true;
        m_stmtSerial = 0;
        m_steps = 0;
        m_pauseSerial = -1;
        m_skipSerial = -1;
        m_frames.clear();
        m_globals.clear();
        m_output.clear();
        m_outputPending.clear();
        m_paused = false;
    } else if (m_paused) {
        // Resume: re-execute from the entry, fast-forwarding to the pause
        // point. The paused statement is executed once (its own breakpoint
        // must not re-fire), so the replay target IS the skip point.
        m_skipSerial = m_pauseSerial;
        m_pauseSerial = -1;
        m_replaying = true;
        m_replayTarget = m_skipSerial;
        m_suppressOutput = true;
        m_frames.clear();
        m_globals.clear();
        m_stmtSerial = 0;
        m_steps = 0;
        m_paused = false;
    }

    try {
        if (!m_topLevelStatements.empty()) {
            InterpFrame script;
            script.id = 1; // frame ids are depth-based (see callUserFunction)
            script.name = "main";
            script.fn = nullptr;
            script.line = m_topLevelStatements.front()->getLocation().line;
            script.scopes.emplace_back();
            m_frames.push_back(script);
            FrameGuard guard(m_frames);
            for (const auto* stmt : m_topLevelStatements) {
                execStmt(*stmt, m_frames.back(), 1);
            }
        }
        if (m_mainFn) {
            InterpFrame dummyCaller;
            dummyCaller.id = 0;
            dummyCaller.name = "<entry>";
            dummyCaller.scopes.emplace_back();
            callUserFunction(*m_mainFn, {}, dummyCaller, 0);
        }
        m_terminated = true;
        outcome.terminated = true;
        return outcome;
    } catch (const InterpPause&) {
        // Snapshot was taken in beforeStatement (before the unwind, while
        // the scopes were still live). Clear the unwound frames.
        m_frames.clear();
        m_paused = true;
        outcome.paused = true;
        return outcome;
    } catch (const InterpReturn&) {
        // A return out of the top-level script: program finished.
        m_terminated = true;
        outcome.terminated = true;
        return outcome;
    } catch (const InterpThrow& t) {
        int throwLine = m_frames.empty() ? 0 : m_frames.back().line;
        m_frames.clear();
        m_terminated = true;
        outcome.terminated = true;
        outcome.error = "Uncaught throw: " + t.message;
        outcome.errorLine = throwLine;
        return outcome;
    } catch (const InterpError& e) {
        m_frames.clear();
        m_terminated = true;
        outcome.terminated = true;
        outcome.error = e.message;
        outcome.errorLine = e.line;
        return outcome;
    }
}

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------

void Interpreter::execBlock(const ast::BlockStmt& block, InterpFrame& frame, int depth) {
    ScopeGuard guard(frame);
    for (const auto& stmt : block.getStatements()) {
        if (stmt) execStmt(*stmt, frame, depth);
    }
}

void Interpreter::execStmt(const ast::StatementNode& stmt, InterpFrame& frame, int depth) {
    if (++m_steps > kMaxSteps) {
        fail("Step limit exceeded (possible infinite loop)", stmt.getLocation().line);
    }
    beforeStatement(stmt, frame, depth, m_runMode);
    ++m_stmtSerial;

    using Kind = ast::ASTNodeKind;
    switch (stmt.getKind()) {
        case Kind::BlockStmt: {
            execBlock(static_cast<const ast::BlockStmt&>(stmt), frame, depth);
            return;
        }
        case Kind::ExprStmt: {
            evalExpr(static_cast<const ast::ExprStmt&>(stmt).getExpr(), frame, depth);
            return;
        }
        case Kind::VarDeclStmt: {
            const auto& v = static_cast<const ast::VarDeclStmt&>(stmt);
            InterpValue value = v.hasInitializer()
                                    ? evalExpr(*v.getInitializer(), frame, depth)
                                    : defaultValue(v.getType());
            if (frame.fn == nullptr) {
                // Top-level (script) declaration: a module-level global,
                // visible from every function.
                m_globals[v.getName()] = value;
            } else {
                frame.scopes.back()[v.getName()] = value;
            }
            return;
        }
        case Kind::IfStmt: {
            const auto& ifs = static_cast<const ast::IfStmt&>(stmt);
            if (evalExpr(ifs.getCondition(), frame, depth).isTruthy()) {
                execStmt(ifs.getThenBranch(), frame, depth);
            } else if (const auto* els = ifs.getElseBranch()) {
                execStmt(*els, frame, depth);
            }
            return;
        }
        case Kind::WhileStmt: {
            const auto& w = static_cast<const ast::WhileStmt&>(stmt);
            while (true) {
                if (++m_steps > kMaxSteps) {
                    fail("Step limit exceeded (possible infinite loop)", stmt.getLocation().line);
                }
                if (!evalExpr(w.getCondition(), frame, depth).isTruthy()) break;
                try {
                    execStmt(w.getBody(), frame, depth);
                } catch (const InterpBreak&) {
                    break;
                } catch (const InterpContinue&) {
                    continue;
                }
            }
            return;
        }
        case Kind::DoWhileStmt: {
            const auto& w = static_cast<const ast::DoWhileStmt&>(stmt);
            while (true) {
                if (++m_steps > kMaxSteps) {
                    fail("Step limit exceeded (possible infinite loop)", stmt.getLocation().line);
                }
                try {
                    execStmt(w.getBody(), frame, depth);
                } catch (const InterpBreak&) {
                    break;
                } catch (const InterpContinue&) {
                    // fall through to the condition check
                }
                if (!evalExpr(w.getCondition(), frame, depth).isTruthy()) break;
            }
            return;
        }
        case Kind::ForStmt: {
            const auto& f = static_cast<const ast::ForStmt&>(stmt);
            ScopeGuard loopScope(frame);
            if (const auto* init = f.getInit()) execStmt(*init, frame, depth);
            while (true) {
                if (++m_steps > kMaxSteps) {
                    fail("Step limit exceeded (possible infinite loop)", stmt.getLocation().line);
                }
                if (f.getCondition() &&
                    !evalExpr(*f.getCondition(), frame, depth).isTruthy()) {
                    break;
                }
                try {
                    execStmt(f.getBody(), frame, depth);
                } catch (const InterpBreak&) {
                    break;
                } catch (const InterpContinue&) {
                    // fall through to the increment
                }
                if (f.getIncrement()) evalExpr(*f.getIncrement(), frame, depth);
            }
            return;
        }
        case Kind::ForInStmt: {
            const auto& f = static_cast<const ast::ForInStmt&>(stmt);
            InterpValue iterable = evalExpr(f.getIterable(), frame, depth);
            std::vector<InterpValue> items;
            if (iterable.kind == InterpValue::Kind::Array) {
                items = iterable.arrayVal;
            } else if (iterable.kind == InterpValue::Kind::Range) {
                for (int64_t i = iterable.intVal; i <= iterable.rangeEnd; ++i) {
                    if (++m_steps > kMaxSteps) {
                        fail("Step limit exceeded (possible infinite loop)",
                             stmt.getLocation().line);
                    }
                    items.push_back(InterpValue::fromInt(i));
                }
            } else if (iterable.kind == InterpValue::Kind::String) {
                for (char c : iterable.stringVal) items.push_back(InterpValue::fromChar(c));
            } else {
                fail("for-in over a non-iterable value (" + iterable.typeName() + ")",
                     stmt.getLocation().line);
            }
            ScopeGuard loopScope(frame);
            for (const auto& item : items) {
                if (++m_steps > kMaxSteps) {
                    fail("Step limit exceeded (possible infinite loop)", stmt.getLocation().line);
                }
                frame.scopes.back()[f.getVarName()] = item;
                try {
                    execStmt(f.getBody(), frame, depth);
                } catch (const InterpBreak&) {
                    break;
                } catch (const InterpContinue&) {
                    continue;
                }
            }
            return;
        }
        case Kind::ReturnStmt: {
            const auto& r = static_cast<const ast::ReturnStmt&>(stmt);
            InterpValue value = r.getValue() ? evalExpr(*r.getValue(), frame, depth)
                                             : InterpValue::voidVal();
            throw InterpReturn{value};
        }
        case Kind::BreakStmt:
            throw InterpBreak{};
        case Kind::ContinueStmt:
            throw InterpContinue{};
        case Kind::ThrowStmt: {
            const auto& t = static_cast<const ast::ThrowStmt&>(stmt);
            InterpValue payload = evalExpr(t.getExpr(), frame, depth);
            throw InterpThrow{payload.toRawString()};
        }
        case Kind::TryCatchStmt: {
            const auto& t = static_cast<const ast::TryCatchStmt&>(stmt);
            auto runFinally = [&]() {
                if (t.hasFinally()) execBlock(*t.getFinallyBlock(), frame, depth);
            };
            try {
                execBlock(t.getTryBlock(), frame, depth);
            } catch (const InterpThrow& thrown) {
                bool matched = false;
                for (const auto& c : t.getCatches()) {
                    // Any catch clause matches: the payload is always
                    // stringified, so there is no runtime type to filter
                    // on — the first catch wins (same as the compiler's
                    // try/catch lowering, Phase 3 slice B). The type
                    // name is a binding annotation only.
                    matched = true;
                    ScopeGuard catchScope(frame);
                    if (!c.getName().empty()) {
                        frame.scopes.back()[c.getName()] = InterpValue::fromString(thrown.message);
                    }
                    try {
                        execBlock(c.getBody(), frame, depth);
                    } catch (const InterpPause&) {
                        throw; // finally runs on resume, when the body completes
                    } catch (...) {
                        runFinally();
                        throw;
                    }
                    break;
                }
                if (!matched) {
                    runFinally();
                    throw;
                }
            } catch (const InterpPause&) {
                throw; // finally runs on resume
            } catch (...) {
                runFinally();
                throw;
            }
            runFinally();
            return;
        }
        case Kind::SwitchStmt:
        case Kind::MatchStmt:
            fail("switch/match statements are not supported in the debugger",
                 stmt.getLocation().line);
        case Kind::FunctionDecl:
            // Nested function declaration: a definition, not an execution.
            return;
        default:
            fail("Statement kind not supported in the debugger", stmt.getLocation().line);
    }
}

void Interpreter::execFunctionBody(const ast::FunctionDecl& fn, InterpFrame& frame, int depth,
                                   InterpValue& retVal) {
    if (fn.isShortForm()) {
        retVal = evalExpr(*fn.getShortBody(), frame, depth);
        return;
    }
    try {
        execBlock(*fn.getBody(), frame, depth);
        retVal = InterpValue::voidVal();
    } catch (const InterpReturn& r) {
        retVal = r.value;
    } catch (const InterpBreak&) {
        fail("break outside a loop", fn.getLocation().line);
    } catch (const InterpContinue&) {
        fail("continue outside a loop", fn.getLocation().line);
    }
}

InterpValue Interpreter::callUserFunction(const ast::FunctionDecl& fn, std::vector<InterpValue> args,
                                         InterpFrame& caller, int depth) {
    if (depth + 1 > kMaxCallDepth) {
        fail("Stack overflow (recursion too deep)", fn.getLocation().line);
    }
    InterpFrame callee;
    // Frame ids are depth-based (1 = outermost), not sequential: replay
    // re-executes from the entry, so a counter would give the same logical
    // frame a new id on every resume and break clients that cache frame
    // ids across stops. Depth is deterministic across replays.
    callee.id = depth + 1;
    callee.name = fn.getName();
    callee.fn = &fn;
    callee.line = fn.hasBody() ? fn.getBody()->getLocation().line : fn.getLocation().line;
    callee.column = fn.getLocation().column;
    callee.returnLine = caller.line;
    callee.scopes.emplace_back();

    const auto& params = fn.getParams();
    for (size_t i = 0; i < params.size(); ++i) {
        InterpValue value;
        if (i < args.size()) {
            value = args[i];
        } else if (params[i].hasDefaultValue()) {
            value = evalExpr(*params[i].getDefaultValue(), caller, depth);
        } else {
            fail("Missing argument '" + params[i].getName() + "' for function '" + fn.getName() + "'",
                 fn.getLocation().line);
        }
        callee.scopes.back()[params[i].getName()] = value;
    }

    m_frames.push_back(callee);
    FrameGuard guard(m_frames);
    InterpValue retVal;
    execFunctionBody(fn, m_frames.back(), depth + 1, retVal);
    return retVal;
}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

InterpValue Interpreter::defaultValue(const std::string& typeName) {
    if (typeName == "int" || typeName == "i64" || typeName == "i32" || typeName == "i16" ||
        typeName == "i8" || typeName == "u64" || typeName == "u32" || typeName == "u16" ||
        typeName == "u8" || typeName == "byte") {
        return InterpValue::fromInt(0);
    }
    if (typeName == "float" || typeName == "double" || typeName == "f64" || typeName == "f32") {
        return InterpValue::fromFloat(0.0);
    }
    if (typeName == "bool") return InterpValue::fromBool(false);
    if (typeName == "string") return InterpValue::fromString("");
    if (typeName == "char") return InterpValue::fromChar('\0');
    return InterpValue::nullVal();
}

std::string Interpreter::stringifyForConcat(const InterpValue& v) const {
    return v.toRawString();
}

InterpValue Interpreter::evalExpr(const ast::ExpressionNode& expr, InterpFrame& frame, int depth) {
    using Kind = ast::ASTNodeKind;
    using TT = lexer::TokenType;
    switch (expr.getKind()) {
        case Kind::LiteralExpr: {
            const auto& lit = static_cast<const ast::LiteralExpr&>(expr);
            switch (lit.getLiteralKind()) {
                case ast::LiteralKind::Integer:
                    return InterpValue::fromInt(std::stoll(lit.getValue()));
                case ast::LiteralKind::Float:
                    return InterpValue::fromFloat(std::stod(lit.getValue()));
                case ast::LiteralKind::String:
                    return InterpValue::fromString(lit.getValue());
                case ast::LiteralKind::Char:
                    return InterpValue::fromChar(lit.getValue().empty() ? '\0' : lit.getValue()[0]);
                case ast::LiteralKind::Boolean:
                    return InterpValue::fromBool(lit.getValue() == "true");
                case ast::LiteralKind::Null:
                    return InterpValue::nullVal();
            }
            fail("Unknown literal kind", expr.getLocation().line);
        }
        case Kind::IdentifierExpr: {
            const auto& id = static_cast<const ast::IdentifierExpr&>(expr);
            InterpValue value;
            if (frame.lookup(id.getName(), value)) return value;
            auto git = m_globals.find(id.getName());
            if (git != m_globals.end()) return git->second;
            fail("Undefined variable '" + id.getName() + "'", expr.getLocation().line);
        }
        case Kind::BinaryExpr: {
            const auto& bin = static_cast<const ast::BinaryExpr&>(expr);
            const TT op = bin.getOp();
            // Short-circuiting logical operators (Phase 1 fixed these in
            // the compiler; the interpreter must match).
            if (op == TT::AmpAmp) {
                bool lhs = evalExpr(bin.getLeft(), frame, depth).isTruthy();
                if (!lhs) return InterpValue::fromBool(false);
                return InterpValue::fromBool(evalExpr(bin.getRight(), frame, depth).isTruthy());
            }
            if (op == TT::PipePipe) {
                bool lhs = evalExpr(bin.getLeft(), frame, depth).isTruthy();
                if (lhs) return InterpValue::fromBool(true);
                return InterpValue::fromBool(evalExpr(bin.getRight(), frame, depth).isTruthy());
            }
            InterpValue lhs = evalExpr(bin.getLeft(), frame, depth);
            InterpValue rhs = evalExpr(bin.getRight(), frame, depth);
            const int line = expr.getLocation().line;

            if (op == TT::DotDot) {
                if (lhs.kind != InterpValue::Kind::Int || rhs.kind != InterpValue::Kind::Int) {
                    fail("Range '..' needs integer bounds", line);
                }
                return InterpValue::fromRange(lhs.intVal, rhs.intVal);
            }
            // String concatenation.
            if (op == TT::Plus &&
                (lhs.kind == InterpValue::Kind::String || rhs.kind == InterpValue::Kind::String ||
                 lhs.kind == InterpValue::Kind::Char || rhs.kind == InterpValue::Kind::Char)) {
                return InterpValue::fromString(stringifyForConcat(lhs) + stringifyForConcat(rhs));
            }
            // Equality.
            if (op == TT::EqualEqual || op == TT::BangEqual) {
                bool equal = valuesEqual(lhs, rhs);
                return InterpValue::fromBool(op == TT::EqualEqual ? equal : !equal);
            }
            // Comparison.
            if (op == TT::Less || op == TT::LessEqual || op == TT::Greater || op == TT::GreaterEqual) {
                bool result = false;
                if (isNumber(lhs) && isNumber(rhs)) {
                    double l = asDouble(lhs), r = asDouble(rhs);
                    result = op == TT::Less ? l < r
                             : op == TT::LessEqual ? l <= r
                             : op == TT::Greater ? l > r
                                                 : l >= r;
                } else if (lhs.kind == InterpValue::Kind::String && rhs.kind == InterpValue::Kind::String) {
                    result = op == TT::Less ? lhs.stringVal < rhs.stringVal
                             : op == TT::LessEqual ? lhs.stringVal <= rhs.stringVal
                             : op == TT::Greater ? lhs.stringVal > rhs.stringVal
                                                 : lhs.stringVal >= rhs.stringVal;
                } else if (lhs.kind == InterpValue::Kind::Char && rhs.kind == InterpValue::Kind::Char) {
                    result = op == TT::Less ? lhs.charVal < rhs.charVal
                             : op == TT::LessEqual ? lhs.charVal <= rhs.charVal
                             : op == TT::Greater ? lhs.charVal > rhs.charVal
                                                 : lhs.charVal >= rhs.charVal;
                } else {
                    fail("Cannot compare " + lhs.typeName() + " with " + rhs.typeName(), line);
                }
                return InterpValue::fromBool(result);
            }
            // Arithmetic.
            if (op == TT::Plus || op == TT::Minus || op == TT::Star || op == TT::Slash ||
                op == TT::Percent) {
                if (!isNumber(lhs) || !isNumber(rhs)) {
                    fail("Arithmetic on non-numbers (" + lhs.typeName() + " " +
                             std::string(lexer::tokenTypeToString(op)) + " " + rhs.typeName() + ")",
                         line);
                }
                if (lhs.kind == InterpValue::Kind::Int && rhs.kind == InterpValue::Kind::Int) {
                    int64_t l = lhs.intVal, r = rhs.intVal;
                    switch (op) {
                        case TT::Plus: return InterpValue::fromInt(l + r);
                        case TT::Minus: return InterpValue::fromInt(l - r);
                        case TT::Star: return InterpValue::fromInt(l * r);
                        case TT::Slash:
                            if (r == 0) fail("Division by zero", line);
                            return InterpValue::fromInt(l / r);
                        case TT::Percent:
                            if (r == 0) fail("Modulo by zero", line);
                            return InterpValue::fromInt(l % r);
                        default: break;
                    }
                }
                double l = asDouble(lhs), r = asDouble(rhs);
                switch (op) {
                    case TT::Plus: return InterpValue::fromFloat(l + r);
                    case TT::Minus: return InterpValue::fromFloat(l - r);
                    case TT::Star: return InterpValue::fromFloat(l * r);
                    case TT::Slash:
                        if (r == 0.0) fail("Division by zero", line);
                        return InterpValue::fromFloat(l / r);
                    case TT::Percent: return InterpValue::fromFloat(std::fmod(l, r));
                    default: break;
                }
                fail("Unsupported arithmetic operator", line);
            }
            // Bitwise (integers only).
            if (op == TT::Amp || op == TT::Pipe || op == TT::Caret || op == TT::LessLess ||
                op == TT::GreaterGreater) {
                if (lhs.kind != InterpValue::Kind::Int || rhs.kind != InterpValue::Kind::Int) {
                    fail("Bitwise operation on non-integers", line);
                }
                switch (op) {
                    case TT::Amp: return InterpValue::fromInt(lhs.intVal & rhs.intVal);
                    case TT::Pipe: return InterpValue::fromInt(lhs.intVal | rhs.intVal);
                    case TT::Caret: return InterpValue::fromInt(lhs.intVal ^ rhs.intVal);
                    case TT::LessLess: return InterpValue::fromInt(lhs.intVal << rhs.intVal);
                    case TT::GreaterGreater: return InterpValue::fromInt(lhs.intVal >> rhs.intVal);
                    default: break;
                }
            }
            fail("Operator not supported in the debugger", line);
        }
        case Kind::UnaryExpr: {
            const auto& un = static_cast<const ast::UnaryExpr&>(expr);
            const TT op = un.getOp();
            const int line = expr.getLocation().line;
            if (op == TT::PlusPlus || op == TT::MinusMinus) {
                // ++/-- on a plain identifier only.
                const auto* id = dynamic_cast<const ast::IdentifierExpr*>(&un.getOperand());
                if (!id) fail("++/-- is only supported on plain variables in the debugger", line);
                InterpValue current;
                if (!frame.lookup(id->getName(), current)) {
                    fail("Undefined variable '" + id->getName() + "'", line);
                }
                if (current.kind != InterpValue::Kind::Int) {
                    fail("++/-- needs an integer variable", line);
                }
                int64_t delta = op == TT::PlusPlus ? 1 : -1;
                InterpValue updated = InterpValue::fromInt(current.intVal + delta);
                frame.assign(id->getName(), updated);
                return un.isPrefix() ? updated : current;
            }
            InterpValue operand = evalExpr(un.getOperand(), frame, depth);
            if (op == TT::Minus) {
                if (operand.kind == InterpValue::Kind::Int) return InterpValue::fromInt(-operand.intVal);
                if (operand.kind == InterpValue::Kind::Float) return InterpValue::fromFloat(-operand.floatVal);
                fail("Unary - on a non-number", line);
            }
            if (op == TT::Bang) return InterpValue::fromBool(!operand.isTruthy());
            if (op == TT::Tilde) {
                if (operand.kind != InterpValue::Kind::Int) fail("~ needs an integer", line);
                return InterpValue::fromInt(~operand.intVal);
            }
            fail("Unary operator not supported in the debugger", line);
        }
        case Kind::TernaryExpr: {
            const auto& t = static_cast<const ast::TernaryExpr&>(expr);
            return evalExpr(t.getCondition(), frame, depth).isTruthy()
                       ? evalExpr(t.getThenExpr(), frame, depth)
                       : evalExpr(t.getElseExpr(), frame, depth);
        }
        case Kind::ArrayExpr: {
            const auto& arr = static_cast<const ast::ArrayExpr&>(expr);
            std::vector<InterpValue> elements;
            for (const auto& el : arr.getElements()) elements.push_back(evalExpr(*el, frame, depth));
            return InterpValue::fromArray(std::move(elements));
        }
        case Kind::MapExpr: {
            const auto& m = static_cast<const ast::MapExpr&>(expr);
            std::map<std::string, InterpValue> entries;
            for (const auto& [keyExpr, valueExpr] : m.getEntries()) {
                InterpValue key = evalExpr(*keyExpr, frame, depth);
                entries[key.toRawString()] = evalExpr(*valueExpr, frame, depth);
            }
            return InterpValue::fromMap(std::move(entries));
        }
        case Kind::IndexExpr: {
            const auto& idx = static_cast<const ast::IndexExpr&>(expr);
            InterpValue object = evalExpr(idx.getObject(), frame, depth);
            InterpValue index = evalExpr(idx.getIndex(), frame, depth);
            const int line = expr.getLocation().line;
            if (object.kind == InterpValue::Kind::Array) {
                if (index.kind != InterpValue::Kind::Int) fail("Array index must be an integer", line);
                if (index.intVal < 0 || index.intVal >= static_cast<int64_t>(object.arrayVal.size())) {
                    fail("Array index out of bounds: " + std::to_string(index.intVal), line);
                }
                return object.arrayVal[static_cast<size_t>(index.intVal)];
            }
            if (object.kind == InterpValue::Kind::String) {
                if (index.kind != InterpValue::Kind::Int) fail("String index must be an integer", line);
                if (index.intVal < 0 || index.intVal >= static_cast<int64_t>(object.stringVal.size())) {
                    fail("String index out of bounds: " + std::to_string(index.intVal), line);
                }
                return InterpValue::fromChar(object.stringVal[static_cast<size_t>(index.intVal)]);
            }
            if (object.kind == InterpValue::Kind::Map) {
                auto it = object.mapVal.find(index.toRawString());
                if (it == object.mapVal.end()) return InterpValue::nullVal();
                return it->second;
            }
            fail("Cannot index a " + object.typeName(), line);
        }
        case Kind::MemberExpr: {
            const auto& mem = static_cast<const ast::MemberExpr&>(expr);
            InterpValue object = evalExpr(mem.getObject(), frame, depth);
            const int line = expr.getLocation().line;
            if (object.kind == InterpValue::Kind::Null && mem.isSafeNav()) return InterpValue::nullVal();
            if (mem.getMember() == "length" || mem.getMember() == "size") {
                if (object.kind == InterpValue::Kind::String) {
                    if (mem.getMember() == "size") {
                        fail("'.size' is not defined on string (use '.length')", line);
                    }
                    return InterpValue::fromInt(static_cast<int64_t>(object.stringVal.size()));
                }
                if (object.kind == InterpValue::Kind::Array) {
                    return InterpValue::fromInt(static_cast<int64_t>(object.arrayVal.size()));
                }
                if (object.kind == InterpValue::Kind::Map) {
                    return InterpValue::fromInt(static_cast<int64_t>(object.mapVal.size()));
                }
                fail("'.length' is not defined on " + object.typeName(), line);
            }
            fail("Field '" + mem.getMember() + "' is not available in the debugger "
                 "(user-defined types are not interpreted)",
                 line);
        }
        case Kind::AssignExpr: {
            const auto& as = static_cast<const ast::AssignExpr&>(expr);
            const int line = expr.getLocation().line;
            InterpValue rhs = evalExpr(as.getValue(), frame, depth);
            const TT op = as.getOp();
            if (op != TT::Equal) {
                // Compound assignment: read the current value first.
                InterpValue current;
                const auto* idTarget = dynamic_cast<const ast::IdentifierExpr*>(&as.getTarget());
                if (idTarget && frame.lookup(idTarget->getName(), current)) {
                    // ok
                } else if (const auto* idxTarget = dynamic_cast<const ast::IndexExpr*>(&as.getTarget())) {
                    current = evalExpr(*idxTarget, frame, depth);
                } else {
                    fail("Compound assignment target not supported in the debugger", line);
                }
                if (op == TT::PlusEqual && (current.kind == InterpValue::Kind::String ||
                                            rhs.kind == InterpValue::Kind::String)) {
                    rhs = InterpValue::fromString(stringifyForConcat(current) + stringifyForConcat(rhs));
                } else if (isNumber(current) && isNumber(rhs)) {
                    if (current.kind == InterpValue::Kind::Int && rhs.kind == InterpValue::Kind::Int) {
                        switch (op) {
                            case TT::PlusEqual: rhs = InterpValue::fromInt(current.intVal + rhs.intVal); break;
                            case TT::MinusEqual: rhs = InterpValue::fromInt(current.intVal - rhs.intVal); break;
                            case TT::StarEqual: rhs = InterpValue::fromInt(current.intVal * rhs.intVal); break;
                            case TT::SlashEqual:
                                if (rhs.intVal == 0) fail("Division by zero", line);
                                rhs = InterpValue::fromInt(current.intVal / rhs.intVal);
                                break;
                            case TT::PercentEqual:
                                if (rhs.intVal == 0) fail("Modulo by zero", line);
                                rhs = InterpValue::fromInt(current.intVal % rhs.intVal);
                                break;
                            default: fail("Assignment operator not supported in the debugger", line);
                        }
                    } else {
                        double l = asDouble(current), r = asDouble(rhs);
                        switch (op) {
                            case TT::PlusEqual: rhs = InterpValue::fromFloat(l + r); break;
                            case TT::MinusEqual: rhs = InterpValue::fromFloat(l - r); break;
                            case TT::StarEqual: rhs = InterpValue::fromFloat(l * r); break;
                            case TT::SlashEqual:
                                if (r == 0.0) fail("Division by zero", line);
                                rhs = InterpValue::fromFloat(l / r);
                                break;
                            case TT::PercentEqual: rhs = InterpValue::fromFloat(std::fmod(l, r)); break;
                            default: fail("Assignment operator not supported in the debugger", line);
                        }
                    }
                } else {
                    fail("Compound assignment on incompatible types", line);
                }
            }
            // Write back.
            if (const auto* idTarget = dynamic_cast<const ast::IdentifierExpr*>(&as.getTarget())) {
                if (frame.assign(idTarget->getName(), rhs)) return rhs;
                auto git = m_globals.find(idTarget->getName());
                if (git != m_globals.end()) {
                    git->second = rhs;
                    return rhs;
                }
                fail("Assignment to undefined variable '" + idTarget->getName() + "'", line);
            }
            if (const auto* idxTarget = dynamic_cast<const ast::IndexExpr*>(&as.getTarget())) {
                // Array/map element assignment needs the container by
                // reference: resolve the container variable first.
                const auto* idObj = dynamic_cast<const ast::IdentifierExpr*>(&idxTarget->getObject());
                if (idObj) {
                    InterpValue container;
                    if (!frame.lookup(idObj->getName(), container)) {
                        fail("Undefined variable '" + idObj->getName() + "'", line);
                    }
                    InterpValue index = evalExpr(idxTarget->getIndex(), frame, depth);
                    if (container.kind == InterpValue::Kind::Array) {
                        if (index.kind != InterpValue::Kind::Int ||
                            index.intVal < 0 ||
                            index.intVal >= static_cast<int64_t>(container.arrayVal.size())) {
                            fail("Array index out of bounds in assignment", line);
                        }
                        container.arrayVal[static_cast<size_t>(index.intVal)] = rhs;
                    } else if (container.kind == InterpValue::Kind::Map) {
                        container.mapVal[index.toRawString()] = rhs;
                    } else {
                        fail("Cannot index-assign a " + container.typeName(), line);
                    }
                    frame.assign(idObj->getName(), container);
                    return rhs;
                }
                fail("Index assignment is only supported on plain variables in the debugger", line);
            }
            fail("Assignment target not supported in the debugger", line);
        }
        case Kind::CallExpr:
            return evalCall(static_cast<const ast::CallExpr&>(expr), frame, depth);
        case Kind::LambdaExpr:
            fail("Lambdas are not supported as values in the debugger", expr.getLocation().line);
        case Kind::NewExpr:
            fail("Classes and 'new' are not supported in the debugger", expr.getLocation().line);
        case Kind::TryExpr:
            fail("'try' expressions are not supported in the debugger", expr.getLocation().line);
        case Kind::AwaitExpr:
            fail("async/await is not supported in the debugger", expr.getLocation().line);
        case Kind::ComptimeExpr:
            fail("Comptime expression was not folded (internal error)", expr.getLocation().line);
        default:
            fail("Expression kind not supported in the debugger", expr.getLocation().line);
    }
}

// ---------------------------------------------------------------------------
// Calls
// ---------------------------------------------------------------------------

namespace {

bool isBuiltinName(const std::string& name) {
    static const char* kBuiltins[] = {"println", "print", "assert", "panic", "len",
                                      "sqrt", "pow", "sin", "cos", "tan",
                                      "asin", "acos", "atan", "atan2", "floor",
                                      "ceil", "round", "abs", "min", "max",
                                      "log", "log10", "log2", "exp"};
    for (const char* b : kBuiltins) {
        if (name == b) return true;
    }
    return false;
}

std::vector<std::string> splitChain(const std::string& chain) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        size_t pos = chain.find('>', start);
        if (pos == std::string::npos) {
            parts.push_back(chain.substr(start));
            break;
        }
        parts.push_back(chain.substr(start, pos - start));
        start = pos + 1;
    }
    return parts;
}

} // namespace

InterpValue Interpreter::evalCall(const ast::CallExpr& call, InterpFrame& frame, int depth) {
    const int line = call.getLocation().line;
    const auto& callee = call.getCallee();

    // Member call: obj.method(args).
    if (const auto* mem = dynamic_cast<const ast::MemberExpr*>(&callee)) {
        const std::string method = mem->getMember();
        std::vector<InterpValue> args;
        for (const auto& arg : call.getArgs()) args.push_back(evalExpr(*arg, frame, depth));

        // Mutating methods on a plain variable (array push/pop, map
        // set/remove): mutate the stored variable, not a copy. Method
        // names match the real SP API (see TypeChecker::visitCall).
        const auto* idObj = dynamic_cast<const ast::IdentifierExpr*>(&mem->getObject());
        if (idObj && (method == "push" || method == "add" || method == "pop" ||
                      method == "removeLast" || method == "set" || method == "remove" ||
                      method == "clear" || method == "removeAt")) {
            InterpValue container;
            if (!frame.lookup(idObj->getName(), container)) {
                fail("Undefined variable '" + idObj->getName() + "'", line);
            }
            if (method == "push" || method == "add") {
                if (container.kind != InterpValue::Kind::Array) fail("'push' needs an array", line);
                if (args.size() != 1) fail("'push' takes exactly 1 argument", line);
                container.arrayVal.push_back(args[0]);
                frame.assign(idObj->getName(), container);
                return InterpValue::voidVal();
            }
            if (method == "pop" || method == "removeLast") {
                if (container.kind != InterpValue::Kind::Array) fail("'pop' needs an array", line);
                if (container.arrayVal.empty()) fail("'pop' on an empty array", line);
                InterpValue back = container.arrayVal.back();
                container.arrayVal.pop_back();
                frame.assign(idObj->getName(), container);
                return back;
            }
            if (method == "clear") {
                if (container.kind == InterpValue::Kind::Array) {
                    container.arrayVal.clear();
                } else if (container.kind == InterpValue::Kind::Map) {
                    container.mapVal.clear();
                } else {
                    fail("'clear' needs an array or map", line);
                }
                frame.assign(idObj->getName(), container);
                return InterpValue::voidVal();
            }
            if (method == "removeAt") {
                if (container.kind != InterpValue::Kind::Array) fail("'removeAt' needs an array", line);
                if (args.size() != 1 || args[0].kind != InterpValue::Kind::Int) {
                    fail("'removeAt' takes exactly 1 integer argument", line);
                }
                if (args[0].intVal < 0 ||
                    args[0].intVal >= static_cast<int64_t>(container.arrayVal.size())) {
                    fail("'removeAt' index out of bounds", line);
                }
                container.arrayVal.erase(container.arrayVal.begin() +
                                         static_cast<size_t>(args[0].intVal));
                frame.assign(idObj->getName(), container);
                return InterpValue::voidVal();
            }
            if (method == "set") {
                if (container.kind == InterpValue::Kind::Map) {
                    if (args.size() != 2) fail("'set' takes exactly 2 arguments", line);
                    container.mapVal[args[0].toRawString()] = args[1];
                } else if (container.kind == InterpValue::Kind::Array) {
                    if (args.size() != 2 || args[0].kind != InterpValue::Kind::Int) {
                        fail("'set' on an array takes (index, value)", line);
                    }
                    if (args[0].intVal < 0 ||
                        args[0].intVal >= static_cast<int64_t>(container.arrayVal.size())) {
                        fail("'set' index out of bounds", line);
                    }
                    container.arrayVal[static_cast<size_t>(args[0].intVal)] = args[1];
                } else {
                    fail("'set' needs an array or map", line);
                }
                frame.assign(idObj->getName(), container);
                return InterpValue::voidVal();
            }
            // remove (map only)
            if (container.kind != InterpValue::Kind::Map) fail("'remove' needs a map", line);
            if (args.size() != 1) fail("'remove' takes exactly 1 argument", line);
            container.mapVal.erase(args[0].toRawString());
            frame.assign(idObj->getName(), container);
            return InterpValue::voidVal();
        }

        InterpValue object = evalExpr(mem->getObject(), frame, depth);
        if (object.kind == InterpValue::Kind::Null && mem->isSafeNav()) return InterpValue::nullVal();

        if (object.kind == InterpValue::Kind::String) {
            const std::string& s = object.stringVal;
            if (method == "length") fail("'length' is a property, not a method", line);
            if (method == "indexOf") {
                if (args.size() != 1) fail("'indexOf' takes exactly 1 argument", line);
                std::string needle = args[0].toRawString();
                size_t pos = s.find(needle);
                return InterpValue::fromInt(pos == std::string::npos ? -1
                                                                      : static_cast<int64_t>(pos));
            }
            if (method == "contains") {
                if (args.size() != 1) fail("'contains' takes exactly 1 argument", line);
                return InterpValue::fromBool(s.find(args[0].toRawString()) != std::string::npos);
            }
            if (method == "startsWith") {
                if (args.size() != 1) fail("'startsWith' takes exactly 1 argument", line);
                const std::string& prefix = args[0].toRawString();
                return InterpValue::fromBool(s.rfind(prefix, 0) == 0);
            }
            if (method == "endsWith") {
                if (args.size() != 1) fail("'endsWith' takes exactly 1 argument", line);
                const std::string& suffix = args[0].toRawString();
                return InterpValue::fromBool(s.size() >= suffix.size() &&
                                             s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0);
            }
            if (method == "substring") {
                if (args.empty() || args.size() > 2 || args[0].kind != InterpValue::Kind::Int) {
                    fail("'substring' takes (start) or (start, end) integer arguments", line);
                }
                int64_t start = args[0].intVal;
                int64_t end = args.size() == 2 ? args[1].intVal : static_cast<int64_t>(s.size());
                if (start < 0 || end < start || end > static_cast<int64_t>(s.size())) {
                    fail("'substring' range out of bounds", line);
                }
                return InterpValue::fromString(s.substr(static_cast<size_t>(start),
                                                        static_cast<size_t>(end - start)));
            }
            if (method == "upper") {
                std::string out = s;
                std::transform(out.begin(), out.end(), out.begin(),
                               [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
                return InterpValue::fromString(out);
            }
            if (method == "lower") {
                std::string out = s;
                std::transform(out.begin(), out.end(), out.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return InterpValue::fromString(out);
            }
            if (method == "reverse") {
                std::string out = s;
                std::reverse(out.begin(), out.end());
                return InterpValue::fromString(out);
            }
            if (method == "isEmpty") {
                if (!args.empty()) fail("'isEmpty' takes no arguments", line);
                return InterpValue::fromBool(s.empty());
            }
            if (method == "charAt") {
                if (args.size() != 1 || args[0].kind != InterpValue::Kind::Int) {
                    fail("'charAt' takes exactly 1 integer argument", line);
                }
                if (args[0].intVal < 0 || args[0].intVal >= static_cast<int64_t>(s.size())) {
                    fail("'charAt' index out of bounds", line);
                }
                return InterpValue::fromChar(s[static_cast<size_t>(args[0].intVal)]);
            }
            if (method == "repeat") {
                if (args.size() != 1 || args[0].kind != InterpValue::Kind::Int) {
                    fail("'repeat' takes exactly 1 integer argument", line);
                }
                if (args[0].intVal < 0) fail("'repeat' needs a non-negative count", line);
                std::string out;
                for (int64_t i = 0; i < args[0].intVal; ++i) out += s;
                return InterpValue::fromString(out);
            }
            if (method == "trim") {
                size_t first = s.find_first_not_of(" \t\r\n");
                if (first == std::string::npos) return InterpValue::fromString("");
                size_t last = s.find_last_not_of(" \t\r\n");
                return InterpValue::fromString(s.substr(first, last - first + 1));
            }
            if (method == "split") {
                if (args.size() != 1) fail("'split' takes exactly 1 argument", line);
                const std::string& sep = args[0].toRawString();
                std::vector<InterpValue> parts;
                if (sep.empty()) {
                    for (char c : s) parts.push_back(InterpValue::fromString(std::string(1, c)));
                } else {
                    size_t start = 0;
                    while (true) {
                        size_t pos = s.find(sep, start);
                        if (pos == std::string::npos) {
                            parts.push_back(InterpValue::fromString(s.substr(start)));
                            break;
                        }
                        parts.push_back(InterpValue::fromString(s.substr(start, pos - start)));
                        start = pos + sep.size();
                    }
                }
                return InterpValue::fromArray(std::move(parts));
            }
            if (method == "replace") {
                if (args.size() != 2) fail("'replace' takes exactly 2 arguments", line);
                const std::string& from = args[0].toRawString();
                const std::string& to = args[1].toRawString();
                std::string out = s;
                size_t pos = 0;
                while (!from.empty() && (pos = out.find(from, pos)) != std::string::npos) {
                    out.replace(pos, from.size(), to);
                    pos += to.size();
                }
                return InterpValue::fromString(out);
            }
            fail("String method '" + method + "' is not supported in the debugger", line);
        }
        if (object.kind == InterpValue::Kind::Array) {
            if (method == "indexOf") {
                if (args.size() != 1) fail("'indexOf' takes exactly 1 argument", line);
                for (size_t i = 0; i < object.arrayVal.size(); ++i) {
                    if (valuesEqual(object.arrayVal[i], args[0])) {
                        return InterpValue::fromInt(static_cast<int64_t>(i));
                    }
                }
                return InterpValue::fromInt(-1);
            }
            if (method == "contains") {
                if (args.size() != 1) fail("'contains' takes exactly 1 argument", line);
                for (const auto& el : object.arrayVal) {
                    if (valuesEqual(el, args[0])) return InterpValue::fromBool(true);
                }
                return InterpValue::fromBool(false);
            }
            if (method == "isEmpty") {
                if (!args.empty()) fail("'isEmpty' takes no arguments", line);
                return InterpValue::fromBool(object.arrayVal.empty());
            }
            if (method == "get") {
                if (args.size() != 1 || args[0].kind != InterpValue::Kind::Int) {
                    fail("'get' takes exactly 1 integer argument", line);
                }
                if (args[0].intVal < 0 ||
                    args[0].intVal >= static_cast<int64_t>(object.arrayVal.size())) {
                    fail("'get' index out of bounds", line);
                }
                return object.arrayVal[static_cast<size_t>(args[0].intVal)];
            }
            fail("Array method '" + method + "' is not supported in the debugger", line);
        }
        if (object.kind == InterpValue::Kind::Map) {
            if (method == "get") {
                if (args.size() != 1) fail("'get' takes exactly 1 argument", line);
                auto it = object.mapVal.find(args[0].toRawString());
                if (it == object.mapVal.end()) return InterpValue::nullVal();
                return it->second;
            }
            if (method == "contains" || method == "containsKey") {
                if (args.size() != 1) fail("'contains' takes exactly 1 argument", line);
                return InterpValue::fromBool(object.mapVal.count(args[0].toRawString()) > 0);
            }
            if (method == "isEmpty") {
                if (!args.empty()) fail("'isEmpty' takes no arguments", line);
                return InterpValue::fromBool(object.mapVal.empty());
            }
            fail("Map method '" + method + "' is not supported in the debugger", line);
        }
        fail("Method '" + method + "' on " + object.typeName() +
                 " is not supported in the debugger (user-defined types are not interpreted)",
             line);
    }

    // Plain call: builtin or user function.
    const auto* id = dynamic_cast<const ast::IdentifierExpr*>(&callee);
    if (!id) {
        fail("Calling a non-function value is not supported in the debugger", line);
    }
    const std::string name = id->getName();
    std::vector<InterpValue> args;
    for (const auto& arg : call.getArgs()) args.push_back(evalExpr(*arg, frame, depth));

    if (isBuiltinName(name)) return callBuiltin(name, std::move(args), call, frame, depth);

    auto fit = m_functions.find(name);
    if (fit == m_functions.end()) {
        fail("Undefined function '" + name +
                 "' (it may need the native runtime, which the debugger cannot run)",
             line);
    }
    // Overloads: pick the first with a matching parameter count (the
    // compiler resolved the exact overload; the interpreter has no access
    // to that resolution, so arity is the discriminator).
    const ast::FunctionDecl* target = nullptr;
    for (const auto* fn : fit->second) {
        if (fn->getParams().size() == args.size()) {
            target = fn;
            break;
        }
    }
    if (!target) {
        // Maybe some parameters have defaults.
        for (const auto* fn : fit->second) {
            if (fn->getParams().size() < args.size()) continue;
            bool ok = true;
            for (size_t i = args.size(); i < fn->getParams().size(); ++i) {
                if (!fn->getParams()[i].hasDefaultValue()) ok = false;
            }
            if (ok) {
                target = fn;
                break;
            }
        }
    }
    if (!target) {
        fail("No overload of '" + name + "' takes " + std::to_string(args.size()) + " argument(s)",
             line);
    }
    return callUserFunction(*target, std::move(args), frame, depth);
}

InterpValue Interpreter::callBuiltin(const std::string& name, std::vector<InterpValue> args,
                                     const ast::CallExpr& call, InterpFrame& /*frame*/,
                                     int /*depth*/) {
    const int line = call.getLocation().line;
    auto needArgs = [&](size_t n) {
        if (args.size() != n) {
            fail("Builtin '" + name + "' takes exactly " + std::to_string(n) + " argument(s)", line);
        }
    };
    auto needNumber = [&](size_t i) {
        if (!isNumber(args[i])) fail("Builtin '" + name + "' needs a number argument", line);
        return asDouble(args[i]);
    };

    if (name == "println") {
        if (args.empty()) {
            emitOutput("");
        } else {
            std::string out;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) out += " ";
                out += args[i].toRawString();
            }
            emitOutput(out);
        }
        return InterpValue::voidVal();
    }
    if (name == "print") {
        std::string out;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) out += " ";
            out += args[i].toRawString();
        }
        emitOutputPartial(out);
        return InterpValue::voidVal();
    }
    if (name == "assert") {
        if (args.empty() || args.size() > 2) fail("'assert' takes 1 or 2 arguments", line);
        if (!args[0].isTruthy()) {
            fail("Assertion failed" + (args.size() == 2 ? ": " + args[1].toRawString() : ""), line);
        }
        return InterpValue::voidVal();
    }
    if (name == "panic") {
        fail("panic" + (args.empty() ? std::string("") : ": " + args[0].toRawString()), line);
    }
    if (name == "len") {
        needArgs(1);
        if (args[0].kind == InterpValue::Kind::String) {
            return InterpValue::fromInt(static_cast<int64_t>(args[0].stringVal.size()));
        }
        if (args[0].kind == InterpValue::Kind::Array) {
            return InterpValue::fromInt(static_cast<int64_t>(args[0].arrayVal.size()));
        }
        if (args[0].kind == InterpValue::Kind::Map) {
            return InterpValue::fromInt(static_cast<int64_t>(args[0].mapVal.size()));
        }
        fail("'len' needs a string, array or map", line);
    }
    if (name == "sqrt" || name == "sin" || name == "cos" || name == "tan" || name == "asin" ||
        name == "acos" || name == "atan" || name == "floor" || name == "ceil" || name == "round" ||
        name == "log" || name == "log10" || name == "log2" || name == "exp") {
        needArgs(1);
        double x = needNumber(0);
        double r = 0.0;
        if (name == "sqrt") r = std::sqrt(x);
        else if (name == "sin") r = std::sin(x);
        else if (name == "cos") r = std::cos(x);
        else if (name == "tan") r = std::tan(x);
        else if (name == "asin") r = std::asin(x);
        else if (name == "acos") r = std::acos(x);
        else if (name == "atan") r = std::atan(x);
        else if (name == "floor") r = std::floor(x);
        else if (name == "ceil") r = std::ceil(x);
        else if (name == "round") r = std::round(x);
        else if (name == "log") r = std::log(x);
        else if (name == "log10") r = std::log10(x);
        else if (name == "log2") r = std::log2(x);
        else if (name == "exp") r = std::exp(x);
        return InterpValue::fromFloat(r);
    }
    if (name == "pow" || name == "atan2") {
        if (args.size() != 2) fail("Builtin '" + name + "' takes exactly 2 arguments", line);
        double x = needNumber(0), y = needNumber(1);
        return InterpValue::fromFloat(name == "pow" ? std::pow(x, y) : std::atan2(x, y));
    }
    if (name == "abs") {
        needArgs(1);
        if (args[0].kind == InterpValue::Kind::Int) {
            return InterpValue::fromInt(args[0].intVal < 0 ? -args[0].intVal : args[0].intVal);
        }
        return InterpValue::fromFloat(std::fabs(needNumber(0)));
    }
    if (name == "min" || name == "max") {
        if (args.size() != 2) fail("Builtin '" + name + "' takes exactly 2 arguments", line);
        if (args[0].kind == InterpValue::Kind::Int && args[1].kind == InterpValue::Kind::Int) {
            return InterpValue::fromInt(name == "min" ? std::min(args[0].intVal, args[1].intVal)
                                                       : std::max(args[0].intVal, args[1].intVal));
        }
        double a = needNumber(0), b = needNumber(1);
        return InterpValue::fromFloat(name == "min" ? std::min(a, b) : std::max(a, b));
    }
    fail("Builtin '" + name + "' is not implemented in the debugger", line);
}

// ---------------------------------------------------------------------------
// evaluate / setVariable (operate on the pause snapshot)
// ---------------------------------------------------------------------------

bool Interpreter::evaluateInFrame(int frameId, const std::string& expression, InterpValue& out,
                                  std::string& error) {
    const InterpFrame* snapshot = nullptr;
    for (const auto& f : m_pausedFrames) {
        if (f.id == frameId) {
            snapshot = &f;
            break;
        }
    }
    if (!snapshot) {
        error = "No paused frame with id " + std::to_string(frameId);
        return false;
    }

    std::ostringstream sink;
    diagnostics::DiagnosticEngine diags(sink);
    lexer::Lexer lexer(expression, "<evaluate>", &diags);
    std::vector<lexer::Token> tokens = lexer.tokenizeAll();
    if (diags.hasErrors()) {
        error = "Cannot parse expression: " + formatDiagnostics(diags);
        return false;
    }
    parser::Parser parser(tokens, "<evaluate>", &diags);
    ast::ExprPtr parsed = parser.parseExpression();
    if (!parsed || diags.hasErrors()) {
        error = "Cannot parse expression: " + formatDiagnostics(diags);
        return false;
    }

    // Save the (paused, unwound) execution state; evaluation runs with
    // pauses disabled, output suppressed, and fresh serial/step counters
    // so it cannot disturb the resume point.
    std::vector<InterpFrame> savedFrames = m_frames;
    int savedSerial = m_stmtSerial;
    int savedSteps = m_steps;
    bool savedSuppress = m_suppressOutput;
    m_frames.clear();
    // Same reserve as run(): frame references must stay valid across
    // nested calls during evaluation.
    m_frames.reserve(static_cast<size_t>(kMaxCallDepth) + 8);
    m_suppressOutput = true;
    m_evaluating = true;

    InterpFrame temp = *snapshot; // scopes are copied; calls get fresh frames
    bool ok = true;
    try {
        out = evalExpr(*parsed, temp, static_cast<int>(m_pausedFrames.size()));
    } catch (const InterpError& e) {
        error = e.message;
        ok = false;
    } catch (const InterpThrow& t) {
        error = "Expression threw: " + t.message;
        ok = false;
    } catch (const InterpPause&) {
        error = "Evaluation paused (internal error)";
        ok = false;
    } catch (const InterpReturn&) {
        error = "Evaluation returned (internal error)";
        ok = false;
    }

    m_frames = std::move(savedFrames);
    m_stmtSerial = savedSerial;
    m_steps = savedSteps;
    m_suppressOutput = savedSuppress;
    m_evaluating = false;
    return ok;
}

bool Interpreter::setVariableOverride(const std::string& frameSignature, const std::string& name,
                                      const InterpValue& value) {
    if (frameSignature == "<globals>") {
        auto it = m_pausedGlobals.find(name);
        if (it == m_pausedGlobals.end()) return false;
        it->second = value;
        m_overrides.push_back(VarOverride{frameSignature, name, value});
        return true;
    }
    // m_pausedFrames is innermost-first; walk outermost -> innermost,
    // building the call chain, and find the frame the signature names.
    std::vector<InterpFrame*> chainOrder;
    for (auto it = m_pausedFrames.rbegin(); it != m_pausedFrames.rend(); ++it) {
        chainOrder.push_back(&*it);
    }
    std::string chain;
    for (size_t i = 0; i < chainOrder.size(); ++i) {
        if (i > 0) chain += ">";
        chain += chainOrder[i]->name;
        if (chain == frameSignature) {
            // Apply to the live snapshot at once (the DAP layer reads it).
            InterpFrame* target = chainOrder[i];
            if (!target->assign(name, value)) return false;
            m_overrides.push_back(VarOverride{frameSignature, name, value});
            return true;
        }
    }
    return false;
}

void Interpreter::applyOverridesAtPausePoint() {
    if (m_overrides.empty()) return;
    std::vector<std::string> current;
    for (const auto& f : m_frames) current.push_back(f.name);
    for (const auto& ov : m_overrides) {
        if (ov.frameSignature == "<globals>") {
            m_globals[ov.name] = ov.value;
            continue;
        }
        std::vector<std::string> target = splitChain(ov.frameSignature);
        if (target.size() > current.size()) continue;
        bool prefix = true;
        for (size_t i = 0; i < target.size(); ++i) {
            if (target[i] != current[i]) {
                prefix = false;
                break;
            }
        }
        if (prefix) m_frames[target.size() - 1].assign(ov.name, ov.value);
    }
}

bool Interpreter::parseLiteralValue(const std::string& text, InterpValue& out) {
    std::string t = text;
    // Trim whitespace.
    size_t first = t.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    size_t last = t.find_last_not_of(" \t\r\n");
    t = t.substr(first, last - first + 1);

    if (t == "null") {
        out = InterpValue::nullVal();
        return true;
    }
    if (t == "true") {
        out = InterpValue::fromBool(true);
        return true;
    }
    if (t == "false") {
        out = InterpValue::fromBool(false);
        return true;
    }
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
        std::string inner;
        for (size_t i = 1; i + 1 < t.size(); ++i) {
            if (t[i] == '\\' && i + 2 < t.size()) {
                switch (t[i + 1]) {
                    case 'n': inner += '\n'; break;
                    case 't': inner += '\t'; break;
                    case 'r': inner += '\r'; break;
                    case '\\': inner += '\\'; break;
                    case '"': inner += '"'; break;
                    default: inner += t[i + 1]; break;
                }
                ++i;
            } else {
                inner += t[i];
            }
        }
        out = InterpValue::fromString(inner);
        return true;
    }
    if (t.size() == 3 && t.front() == '\'' && t.back() == '\'') {
        out = InterpValue::fromChar(t[1]);
        return true;
    }
    try {
        size_t used = 0;
        long long iv = std::stoll(t, &used);
        if (used == t.size()) {
            out = InterpValue::fromInt(static_cast<int64_t>(iv));
            return true;
        }
    } catch (...) {
    }
    try {
        size_t used = 0;
        double fv = std::stod(t, &used);
        if (used == t.size()) {
            out = InterpValue::fromFloat(fv);
            return true;
        }
    } catch (...) {
    }
    return false;
}

} // namespace sp::dap
