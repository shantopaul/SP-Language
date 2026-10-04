#pragma once

#include <string>
#include <cstdint>

namespace sp::diagnostics {

struct SourceLocation {
    std::string filename;
    uint32_t line{1};
    uint32_t column{1};

    SourceLocation() = default;
    SourceLocation(std::string file, uint32_t l, uint32_t c)
        : filename(std::move(file)), line(l), column(c) {}

    [[nodiscard]] std::string toString() const;
};

struct SourceRange {
    SourceLocation start;
    SourceLocation end;

    SourceRange() = default;
    SourceRange(SourceLocation s, SourceLocation e)
        : start(std::move(s)), end(std::move(e)) {}
};

} // namespace sp::diagnostics
