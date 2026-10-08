#include "sp/ast/Edition.h"

namespace sp::ast {

Edition parseEdition(std::string_view str) noexcept {
    if (str == "2028" || str == "v2" || str == "2.0") {
        return Edition::Edition2028;
    }
    return Edition::Edition2026;
}

std::string_view editionToString(Edition edition) noexcept {
    switch (edition) {
        case Edition::Edition2028: return "2028";
        case Edition::Edition2026:
        default:
            return "2026";
    }
}

bool isEditionAtLeast(Edition current, Edition required) noexcept {
    return static_cast<int>(current) >= static_cast<int>(required);
}

} // namespace sp::ast
