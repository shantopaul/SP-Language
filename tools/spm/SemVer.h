#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace sp::spm {

struct Version {
    int major{0};
    int minor{0};
    int patch{0};
    std::string prerelease;
    std::string build;

    static std::optional<Version> parse(std::string_view str);

    std::string toString() const;

    bool operator==(const Version& other) const;
    bool operator!=(const Version& other) const { return !(*this == other); }
    bool operator<(const Version& other) const;
    bool operator<=(const Version& other) const { return *this < other || *this == other; }
    bool operator>(const Version& other) const { return !(*this <= other); }
    bool operator>=(const Version& other) const { return !(*this < other); }

private:
    static int comparePrerelease(std::string_view a, std::string_view b);
};

enum class ConstraintOp {
    Exact,       // = or ==
    Compatible,  // ^ (caret)
    Tilde,       // ~
    Greater,     // >
    GreaterEq,   // >=
    Less,        // <
    LessEq,      // <=
    Wildcard     // *
};

struct VersionConstraint {
    ConstraintOp op{ConstraintOp::Wildcard};
    Version version;

    bool matches(const Version& v) const;
};

class VersionReq {
public:
    VersionReq() = default;

    static std::optional<VersionReq> parse(std::string_view str);

    bool matches(const Version& v) const;
    std::string toString() const { return m_originalStr; }
    bool isAny() const { return m_constraints.empty(); }

private:
    std::string m_originalStr;
    std::vector<VersionConstraint> m_constraints;
};

} // namespace sp::spm
