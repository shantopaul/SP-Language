#pragma once

#include <string_view>
#include <string>

namespace sp::ast {

enum class Edition {
    Edition2026, // SP 1.x Baseline (stable enterprise compatibility)
    Edition2028  // SP 2.0 Modern (comptime, async/await coroutines, strict memory safety)
};

[[nodiscard]] Edition parseEdition(std::string_view str) noexcept;
[[nodiscard]] std::string_view editionToString(Edition edition) noexcept;
[[nodiscard]] bool isEditionAtLeast(Edition current, Edition required) noexcept;

[[nodiscard]] inline bool editionSupportsComptime(Edition edition) noexcept {
    return isEditionAtLeast(edition, Edition::Edition2028);
}

[[nodiscard]] inline bool editionSupportsCoroutines(Edition edition) noexcept {
    return isEditionAtLeast(edition, Edition::Edition2028);
}

[[nodiscard]] inline bool editionEnforcesStrictUnsafe(Edition edition) noexcept {
    return isEditionAtLeast(edition, Edition::Edition2028);
}

} // namespace sp::ast
