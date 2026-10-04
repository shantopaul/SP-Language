#include "sp/diagnostics/SourceLocation.h"
#include <sstream>

namespace sp::diagnostics {

std::string SourceLocation::toString() const {
    std::ostringstream oss;
    oss << (filename.empty() ? "<unknown>" : filename) << ":" << line << ":" << column;
    return oss.str();
}

} // namespace sp::diagnostics
