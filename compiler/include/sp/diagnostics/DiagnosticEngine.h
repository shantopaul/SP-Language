#pragma once

#include "sp/diagnostics/SourceLocation.h"
#include <string>
#include <vector>
#include <string_view>
#include <iostream>

namespace sp::diagnostics {

enum class DiagnosticSeverity {
    Error,
    Warning,
    Info,
    Hint
};

struct Diagnostic {
    DiagnosticSeverity severity;
    std::string code;
    std::string message;
    SourceLocation location;
    std::string hint;

    Diagnostic(DiagnosticSeverity sev, std::string c, std::string msg, SourceLocation loc, std::string h = "")
        : severity(sev), code(std::move(c)), message(std::move(msg)), location(std::move(loc)), hint(std::move(h)) {}
};

class DiagnosticEngine {
public:
    explicit DiagnosticEngine(std::ostream& output = std::cerr);

    void report(DiagnosticSeverity severity, std::string_view code, std::string_view message, const SourceLocation& loc, std::string_view hint = "");
    void reportError(std::string_view code, std::string_view message, const SourceLocation& loc, std::string_view hint = "");
    void reportWarning(std::string_view code, std::string_view message, const SourceLocation& loc, std::string_view hint = "");

    [[nodiscard]] bool hasErrors() const noexcept { return m_errorCount > 0; }
    [[nodiscard]] size_t errorCount() const noexcept { return m_errorCount; }
    [[nodiscard]] size_t warningCount() const noexcept { return m_warningCount; }
    [[nodiscard]] const std::vector<Diagnostic>& getDiagnostics() const noexcept { return m_diagnostics; }

    void clear();

private:
    std::ostream& m_out;
    std::vector<Diagnostic> m_diagnostics;
    size_t m_errorCount{0};
    size_t m_warningCount{0};

    void printDiagnostic(const Diagnostic& diag);
};

} // namespace sp::diagnostics
