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

    return m_diagnostics.errorCount() == errCountBefore;
}

} // namespace sp::sema
