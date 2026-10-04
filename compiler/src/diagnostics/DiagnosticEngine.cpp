#include "sp/diagnostics/DiagnosticEngine.h"

namespace sp::diagnostics {

namespace {
    constexpr const char* ANSI_RESET  = "\033[0m";
    constexpr const char* ANSI_RED    = "\033[1;31m";
    constexpr const char* ANSI_YELLOW = "\033[1;33m";
    constexpr const char* ANSI_BLUE   = "\033[1;34m";
    constexpr const char* ANSI_CYAN   = "\033[1;36m";
    constexpr const char* ANSI_GREEN  = "\033[32m";
    constexpr const char* ANSI_BOLD   = "\033[1m";
}

DiagnosticEngine::DiagnosticEngine(std::ostream& output)
    : m_out(output) {}

void DiagnosticEngine::report(DiagnosticSeverity severity, std::string_view code, std::string_view message, const SourceLocation& loc, std::string_view hint) {
    if (severity == DiagnosticSeverity::Error) {
        m_errorCount++;
    } else if (severity == DiagnosticSeverity::Warning) {
        m_warningCount++;
    }

    Diagnostic diag(severity, std::string(code), std::string(message), loc, std::string(hint));
    m_diagnostics.push_back(diag);
    printDiagnostic(diag);
}

void DiagnosticEngine::reportError(std::string_view code, std::string_view message, const SourceLocation& loc, std::string_view hint) {
    report(DiagnosticSeverity::Error, code, message, loc, hint);
}

void DiagnosticEngine::reportWarning(std::string_view code, std::string_view message, const SourceLocation& loc, std::string_view hint) {
    report(DiagnosticSeverity::Warning, code, message, loc, hint);
}

void DiagnosticEngine::clear() {
    m_diagnostics.clear();
    m_errorCount = 0;
    m_warningCount = 0;
}

void DiagnosticEngine::printDiagnostic(const Diagnostic& diag) {
    const char* color = ANSI_RED;
    const char* label = "error";

    switch (diag.severity) {
        case DiagnosticSeverity::Error:
            color = ANSI_RED;
            label = "error";
            break;
        case DiagnosticSeverity::Warning:
            color = ANSI_YELLOW;
            label = "warning";
            break;
        case DiagnosticSeverity::Info:
            color = ANSI_BLUE;
            label = "info";
            break;
        case DiagnosticSeverity::Hint:
            color = ANSI_CYAN;
            label = "hint";
            break;
    }

    m_out << color << label;
    if (!diag.code.empty()) {
        m_out << "[" << diag.code << "]";
    }
    m_out << ": " << ANSI_RESET << ANSI_BOLD << diag.message << ANSI_RESET << "\n";

    m_out << "  --> " << diag.location.toString() << "\n";

    if (!diag.hint.empty()) {
        m_out << "   = " << ANSI_GREEN << "help: " << ANSI_RESET << diag.hint << "\n";
    }
}

} // namespace sp::diagnostics
