#pragma once

// Phase 4 slice C (pro-4.0.0): a real tree-walking interpreter for the
// debugger. DebugSession used to be a line-walking simulator: it never
// executed the program, it regex-guessed variable values out of the
// source text (so `let z = x + y;` showed the value "x + y"), loops ran
// once, function calls never bound parameters, and `evaluate("x + 1")`
// echoed the expression back as its own value.
//
// This interpreter executes the program for real, on the compiler's own
// AST (Lexer -> Parser -> ModuleLoader -> SemanticAnalyzer, the same
// pipeline `spc` uses), with real values, real loops, real calls, and a
// real call stack. Pause/resume is replay-based: when execution pauses,
// the interpreter state is snapshotted and the C++ stack unwinds; on
// resume the program is re-executed from the entry point, fast-forwarding
// (no pauses, no output) to the statement it paused before, then
// continuing in the requested mode. The program must therefore be
// deterministic across replays (no time/random/input) — documented in
// docs/v2/PHASE-33-PRO-4.0.0.md. `println` output is captured and
// re-emitted exactly once (replay output is discarded).
//
// Supported subset (anything else is a loud InterpError, never a silent
// wrong value): let/const/var declarations, assignment (+= etc.),
// if/else, while, do-while, for, for-in over arrays and int ranges,
// return/break/continue, try/catch/finally + throw (scalar payloads,
// stringified), ternary, arrays, maps, indexing, string concat and the
// `.length` property, string methods (upper/lower/trim/reverse/repeat/
// contains/startsWith/endsWith/isEmpty/indexOf/replace/substring/charAt/
// split), array methods (push/add/pop/removeLast/get/set/contains/
// indexOf/isEmpty/clear/removeAt, .length/.size), map methods (get/set/
// remove/clear/contains/containsKey/isEmpty, .length/.size), builtins
// (println/print, assert, panic, len, math). NOT supported (loud error):
// classes and `new`, methods on user types, lambdas as values (so
// map/filter/reduce on arrays), async/await, `expr?`, switch/match, I/O
// builtins (readln, fs), extern "C" calls, stdlib functions that need
// the native runtime.

#include "sp/ast/ASTNode.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Expression.h"
#include "sp/ast/Statement.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sp::dap {

// ---------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------

struct InterpValue {
    enum class Kind { Void, Int, Float, Bool, String, Char, Array, Map, Null, Range };

    Kind kind{Kind::Void};
    int64_t intVal{0};
    double floatVal{0.0};
    bool boolVal{false};
    std::string stringVal;
    char charVal{'\0'};
    std::vector<InterpValue> arrayVal;
    std::map<std::string, InterpValue> mapVal;
    int64_t rangeEnd{0}; // Range: intVal is the start (inclusive), rangeEnd the end (inclusive)

    static InterpValue fromInt(int64_t v);
    static InterpValue fromFloat(double v);
    static InterpValue fromBool(bool v);
    static InterpValue fromString(std::string v);
    static InterpValue fromChar(char v);
    static InterpValue fromArray(std::vector<InterpValue> v);
    static InterpValue fromMap(std::map<std::string, InterpValue> v);
    static InterpValue fromRange(int64_t start, int64_t end);
    static InterpValue nullVal();
    static InterpValue voidVal();

    // DAP display value. Strings are shown quoted, like a debugger shows
    // them; arrays/maps as [1, 2] / {"a": 1}.
    [[nodiscard]] std::string display() const;
    // DAP type name: int, float, bool, string, char, int[], string{}, void, null.
    [[nodiscard]] std::string typeName() const;
    [[nodiscard]] bool isTruthy() const;
    // Raw string form (no quotes) for string concatenation / println.
    [[nodiscard]] std::string toRawString() const;
};

// ---------------------------------------------------------------------------
// Control-flow signals (thrown inside the interpreter, caught at frame
// boundaries). InterpPause carries no data: the pause snapshot lives on
// the Interpreter.
// ---------------------------------------------------------------------------

struct InterpReturn {
    InterpValue value;
};
struct InterpBreak {};
struct InterpContinue {};
struct InterpThrow {
    std::string message;
};
struct InterpPause {};
struct InterpError {
    std::string message;
    int line{0};
};

// One call-stack frame. scopes[0] holds parameters; later entries are
// block scopes (innermost last). A snapshot of this struct is what the
// DAP layer shows for scopes/variables.
struct InterpFrame {
    int id{0};
    std::string name{"main"};
    const ast::FunctionDecl* fn{nullptr}; // nullptr for the top-level script frame
    int line{1};                          // line of the statement about to execute
    int column{1};
    std::string file; // file of the statement about to execute
    std::vector<std::map<std::string, InterpValue>> scopes;
    // Line in the caller to resume at once this frame returns (0 = unknown).
    int returnLine{0};

    [[nodiscard]] std::string signature() const; // the frame's own name ("main", "add", ...)
    bool lookup(const std::string& name, InterpValue& out) const;
    // Define in the innermost scope, or assign to the nearest scope that
    // already has the name. Returns false when the name is not defined.
    bool assign(const std::string& name, const InterpValue& value);
};

// How a run slice should pause.
enum class RunMode {
    Continue,   // pause only at breakpoints
    StepOver,   // pause at the next statement at depth <= pause depth
    StepIn,     // pause at the very next statement (any depth)
    StepOut,    // pause at the next statement at depth < pause depth
    PauseAtFirst // pause at the first statement (stopOnEntry)
};

struct RunOutcome {
    bool paused{false};
    bool terminated{false};
    std::string error; // non-empty when the program died with a runtime error
    int errorLine{0};
};

// A recorded setVariable override, re-applied when a replay reaches the
// pause point (see Interpreter::setVariableOverride).
struct VarOverride {
    std::string frameSignature; // e.g. "main" or "main>add"
    std::string name;
    InterpValue value;
};

class Interpreter {
public:
    Interpreter() = default;

    // Runs the full front-end pipeline (lex, parse, module load, sema).
    // Returns false and fills `error` when the program cannot even load
    // (file missing, syntax error, sema error, no main and no top-level
    // statements). `source` may be given directly (tests) instead of
    // reading `path` from disk.
    bool load(const std::string& path, const std::string& source, std::string& error);

    // Breakpoint lines (statement start lines, from the AST), per file.
    // A breakpoint only fires for statements in its own file.
    [[nodiscard]] const std::vector<int>& executableLines() const {
        static const std::vector<int> kEmpty;
        auto it = m_executableLinesByFile.find(m_programPath);
        return it == m_executableLinesByFile.end() ? kEmpty : it->second;
    }
    [[nodiscard]] const std::vector<int>& executableLinesFor(const std::string& path) const {
        static const std::vector<int> kEmpty;
        auto it = m_executableLinesByFile.find(path);
        return it == m_executableLinesByFile.end() ? kEmpty : it->second;
    }

    void setBreakpoints(const std::string& path, std::vector<int> lines) {
        m_breakpointsByFile[path] = std::move(lines);
    }
    void setBreakpointsEnabled(bool enabled) { m_breakpointsEnabled = enabled; }

    // Runs a slice. First call starts execution; later calls resume from
    // the pause point (replay-based, see the file header). The mode only
    // matters when resuming or starting; Continue from a pause re-arms
    // breakpoints (the paused statement itself is executed once without
    // re-triggering its own breakpoint).
    RunOutcome run(RunMode mode);

    [[nodiscard]] bool isPaused() const { return m_paused; }
    [[nodiscard]] bool isTerminated() const { return m_terminated; }
    [[nodiscard]] const std::string& pauseReason() const { return m_pauseReason; }
    [[nodiscard]] int pauseSerial() const { return m_pauseSerial; }

    // Snapshot taken at the pause point (innermost frame first).
    [[nodiscard]] const std::vector<InterpFrame>& pausedFrames() const { return m_pausedFrames; }
    // Top-level (global) variables at the pause point.
    [[nodiscard]] const std::map<std::string, InterpValue>& pausedGlobals() const { return m_pausedGlobals; }
    // Discard the pause snapshot (used when the program terminates, so a
    // stale snapshot can never be inspected afterwards).
    void clearPausedSnapshot() {
        m_pausedFrames.clear();
        m_pausedGlobals.clear();
    }

    // Output captured since the last drain (println/print lines, each
    // without the trailing newline). Replay output is discarded.
    std::vector<std::string> drainOutput();

    // Evaluate an expression in a paused frame (by frame id in the
    // snapshot). Pure evaluation; calls run to completion with no
    // breakpoints. Returns false + error when it cannot be evaluated.
    bool evaluateInFrame(int frameId, const std::string& expression, InterpValue& out, std::string& error);

    // Record a setVariable override to re-apply at the pause point on
    // resume. The override is also applied to the live snapshot at once.
    // Returns false when the frame/variable does not exist in the snapshot.
    // The special frameSignature "<globals>" targets module-level globals.
    bool setVariableOverride(const std::string& frameSignature, const std::string& name, const InterpValue& value);

    // Parse a setVariable value string (a literal: int, float, bool,
    // "string", char, null). Returns false when it is not a literal.
    static bool parseLiteralValue(const std::string& text, InterpValue& out);

private:
    // --- loaded program ---
    std::string m_programPath;
    std::unique_ptr<ast::Program> m_program;
    std::map<std::string, std::vector<const ast::FunctionDecl*>> m_functions;
    std::map<std::string, std::vector<int>> m_executableLinesByFile;
    std::vector<const ast::StatementNode*> m_topLevelStatements;
    const ast::FunctionDecl* m_mainFn{nullptr};

    // --- execution state ---
    std::vector<InterpFrame> m_frames;
    std::map<std::string, InterpValue> m_globals;
    std::vector<std::string> m_output;
    std::string m_outputPending; // text printed by `print` without a newline yet
    std::map<std::string, std::vector<int>> m_breakpointsByFile;
    bool m_breakpointsEnabled{true};

    bool m_paused{false};
    bool m_terminated{false};
    bool m_started{false};
    std::string m_pauseReason;
    int m_pauseSerial{-1}; // serial of the statement paused before
    int m_pauseDepth{0};
    int m_skipSerial{-1}; // on resume: the pause-point statement, executed once without pausing
    RunMode m_runMode{RunMode::Continue};
    int m_stmtSerial{0};   // serial of the next statement to execute
    int m_steps{0};        // statements + loop iterations executed (limit guard)
    bool m_replaying{false};
    int m_replayTarget{-1};
    bool m_suppressOutput{false};
    bool m_evaluating{false}; // evaluateInFrame: pauses disabled, output suppressed
    std::vector<InterpFrame> m_pausedFrames;
    std::map<std::string, InterpValue> m_pausedGlobals;
    std::vector<VarOverride> m_overrides;

    void collectExecutableLines();
    void collectFromBlock(const ast::BlockStmt& block);
    void collectFromStmt(const ast::StatementNode& stmt);

    // The recursive engine. execBlock/execStmt throw InterpReturn /
    // InterpBreak / InterpContinue / InterpThrow / InterpPause /
    // InterpError. evalExpr throws InterpError / InterpThrow.
    void execBlock(const ast::BlockStmt& block, InterpFrame& frame, int depth);
    void execStmt(const ast::StatementNode& stmt, InterpFrame& frame, int depth);
    void execFunctionBody(const ast::FunctionDecl& fn, InterpFrame& frame, int depth, InterpValue& retVal);
    InterpValue evalExpr(const ast::ExpressionNode& expr, InterpFrame& frame, int depth);
    InterpValue evalCall(const ast::CallExpr& call, InterpFrame& frame, int depth);
    InterpValue callUserFunction(const ast::FunctionDecl& fn, std::vector<InterpValue> args,
                                 InterpFrame& caller, int depth);
    InterpValue callBuiltin(const std::string& name, std::vector<InterpValue> args,
                            const ast::CallExpr& call, InterpFrame& frame, int depth);

    // Pause check run before executing a statement. Throws InterpPause
    // when this slice must stop here. `stmt` is the statement about to
    // run; its serial is m_stmtSerial (not yet incremented).
    void beforeStatement(const ast::StatementNode& stmt, InterpFrame& frame, int depth, RunMode mode);
    // Snapshot the live frames (innermost first) + globals at the pause
    // point, BEFORE the InterpPause unwinds (scope/frame guards pop on
    // unwind, so the snapshot must be taken here, not in run()'s catch).
    void takePauseSnapshot(int serial, int depth, const std::string& reason);
    // Re-apply recorded setVariable overrides when a replay reaches the
    // pause point.
    void applyOverridesAtPausePoint();

    void emitOutput(const std::string& line);
    void emitOutputPartial(const std::string& text);
    [[noreturn]] void fail(const std::string& message, int line);
    InterpValue defaultValue(const std::string& typeName);
    std::string stringifyForConcat(const InterpValue& v) const;
};

} // namespace sp::dap
