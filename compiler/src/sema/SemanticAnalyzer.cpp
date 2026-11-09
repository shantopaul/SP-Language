#include "sp/sema/SemanticAnalyzer.h"

namespace sp::sema {

bool SemanticAnalyzer::analyze(const ast::Program& program) {
    size_t errCountBefore = m_diagnostics.errorCount();

    // Pass 1 & 2: Name & Type Declaration and Inheritance resolution
    m_nameResolver.resolve(program);
    if (m_diagnostics.errorCount() > errCountBefore) {
        return false;
    }

    // Pass 3: Detailed Type Checking & Inference
    m_typeChecker.check(program);

    // Phase 6 slice C: the return value covers errors that were already
    // in the diagnostics when analyze() was called — most importantly
    // parse errors. Before this, analyze() only reported whether sema
    // added NEW errors, so it returned true for a program that failed
    // to parse (as long as sema added nothing), and callers that
    // checked only the return value (spm BuildOrchestrator) would
    // proceed to codegen on a broken AST. The passes still run even
    // when the input already has errors (their side effects — symbol
    // tables, resolved types — are what the LSP and SymbolIndex use,
    // and they ignore the return value); only the boolean changed.
    // Diagnostics only accumulate, so errorCount() == 0 exactly when
    // the input was clean and sema added nothing.
    return m_diagnostics.errorCount() == 0;
}

} // namespace sp::sema
