#include "SemVer.h"
#include <charconv>
#include <sstream>
#include <cctype>
#include <algorithm>

namespace sp::spm {

namespace {

std::string trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return std::string(sv);
}

std::vector<std::string_view> split(std::string_view s, char delim) {
    std::vector<std::string_view> parts;
    size_t start = 0;
    while (start < s.size()) {
        size_t end = s.find(delim, start);
        if (end == std::string_view::npos) {
            parts.push_back(s.substr(start));
            break;
        }
        parts.push_back(s.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

} // namespace

std::optional<Version> Version::parse(std::string_view str) {
    std::string trimmed = trim(str);
    if (trimmed.empty()) return std::nullopt;

    // Check for build metadata '+'
    std::string_view mainPart = trimmed;
    std::string buildMeta;
    size_t plusPos = mainPart.find('+');
    if (plusPos != std::string_view::npos) {
        buildMeta = mainPart.substr(plusPos + 1);
        mainPart = mainPart.substr(0, plusPos);
    }

    // Check for prerelease '-'
    std::string prerelease;
    size_t dashPos = mainPart.find('-');
    if (dashPos != std::string_view::npos) {
        prerelease = mainPart.substr(dashPos + 1);
        mainPart = mainPart.substr(0, dashPos);
    }

    auto nums = split(mainPart, '.');
    if (nums.empty() || nums.size() > 3) return std::nullopt;

    Version v;
    v.prerelease = std::move(prerelease);
    v.build = std::move(buildMeta);

    auto parseInt = [](std::string_view part, int& out) -> bool {
        if (part.empty()) return false;
        for (char c : part) {
            if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        }
        auto res = std::from_chars(part.data(), part.data() + part.size(), out);
        return res.ec == std::errc();
    };

    if (!parseInt(nums[0], v.major)) return std::nullopt;
    if (nums.size() >= 2) {
        if (!parseInt(nums[1], v.minor)) return std::nullopt;
    }
    if (nums.size() >= 3) {
        if (!parseInt(nums[2], v.patch)) return std::nullopt;
    }

    return v;
}

std::string Version::toString() const {
    std::ostringstream oss;
    oss << major << "." << minor << "." << patch;
    if (!prerelease.empty()) {
        oss << "-" << prerelease;
    }
    if (!build.empty()) {
        oss << "+" << build;
    }
    return oss.str();
}

int Version::comparePrerelease(std::string_view a, std::string_view b) {
    // Both are non-empty
    auto aParts = split(a, '.');
    auto bParts = split(b, '.');

    size_t count = std::min(aParts.size(), bParts.size());
    for (size_t i = 0; i < count; ++i) {
        auto ap = aParts[i];
        auto bp = bParts[i];

        bool aIsNum = !ap.empty() && std::all_of(ap.begin(), ap.end(), [](char c) { return std::isdigit(c); });
        bool bIsNum = !bp.empty() && std::all_of(bp.begin(), bp.end(), [](char c) { return std::isdigit(c); });

        if (aIsNum && bIsNum) {
            int ai = 0, bi = 0;
            std::from_chars(ap.data(), ap.data() + ap.size(), ai);
            std::from_chars(bp.data(), bp.data() + bp.size(), bi);
            if (ai != bi) return ai < bi ? -1 : 1;
        } else if (aIsNum && !bIsNum) {
            return -1; // numeric has lower precedence than string
        } else if (!aIsNum && bIsNum) {
            return 1;
        } else {
            if (ap != bp) return ap < bp ? -1 : 1;
        }
    }

    if (aParts.size() != bParts.size()) {
        return aParts.size() < bParts.size() ? -1 : 1;
    }
    return 0;
}

bool Version::operator==(const Version& other) const {
    return major == other.major &&
           minor == other.minor &&
           patch == other.patch &&
           prerelease == other.prerelease;
}

bool Version::operator<(const Version& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    if (patch != other.patch) return patch < other.patch;

    // Pre-release version has lower precedence than normal version
    if (prerelease.empty() && !other.prerelease.empty()) return false;
    if (!prerelease.empty() && other.prerelease.empty()) return true;
    if (!prerelease.empty() && !other.prerelease.empty()) {
        return comparePrerelease(prerelease, other.prerelease) < 0;
    }

    return false;
}

bool VersionConstraint::matches(const Version& v) const {
    switch (op) {
        case ConstraintOp::Wildcard:
            return true;

        case ConstraintOp::Exact:
            return v == version;

        case ConstraintOp::Greater:
            return v > version;

        case ConstraintOp::GreaterEq:
            return v >= version;

        case ConstraintOp::Less:
            return v < version;

        case ConstraintOp::LessEq:
            return v <= version;

        case ConstraintOp::Compatible: { // ^
            if (v < version) return false;
            if (version.major > 0) {
                return v.major == version.major;
            }
            if (version.minor > 0) {
                return v.major == 0 && v.minor == version.minor;
            }
            return v.major == 0 && v.minor == 0 && v.patch == version.patch;
        }

        case ConstraintOp::Tilde: { // ~
            if (v < version) return false;
            return v.major == version.major && v.minor == version.minor;
        }
    }
    return false;
}

std::optional<VersionReq> VersionReq::parse(std::string_view str) {
    VersionReq req;
    req.m_originalStr = trim(str);

    if (req.m_originalStr.empty() || req.m_originalStr == "*") {
        return req;
    }

    // Split by comma if present: e.g. ">= 1.0.0, < 2.0.0"
    auto parts = split(req.m_originalStr, ',');
    for (auto partView : parts) {
        std::string part = trim(partView);
        if (part.empty() || part == "*") {
            continue;
        }

        ConstraintOp op = ConstraintOp::Compatible; // Default compatible
        std::string_view verStr = part;

        if (verStr.starts_with(">=")) {
            op = ConstraintOp::GreaterEq;
            verStr.remove_prefix(2);
        } else if (verStr.starts_with("<=")) {
            op = ConstraintOp::LessEq;
            verStr.remove_prefix(2);
        } else if (verStr.starts_with("==")) {
            op = ConstraintOp::Exact;
            verStr.remove_prefix(2);
        } else if (verStr.starts_with("=")) {
            op = ConstraintOp::Exact;
            verStr.remove_prefix(1);
        } else if (verStr.starts_with(">")) {
            op = ConstraintOp::Greater;
            verStr.remove_prefix(1);
        } else if (verStr.starts_with("<")) {
            op = ConstraintOp::Less;
            verStr.remove_prefix(1);
        } else if (verStr.starts_with("^")) {
            op = ConstraintOp::Compatible;
            verStr.remove_prefix(1);
        } else if (verStr.starts_with("~")) {
            op = ConstraintOp::Tilde;
            verStr.remove_prefix(1);
        }

        std::string cleanedVer = trim(verStr);
        auto parsedVer = Version::parse(cleanedVer);
        if (!parsedVer) {
            return std::nullopt;
        }

        req.m_constraints.push_back(VersionConstraint{op, *parsedVer});
    }

    return req;
}

bool VersionReq::matches(const Version& v) const {
    if (m_constraints.empty()) {
        return true;
    }
    for (const auto& c : m_constraints) {
        if (!c.matches(v)) {
            return false;
        }
    }
    return true;
}

} // namespace sp::spm
