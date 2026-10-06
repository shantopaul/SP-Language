#pragma once

#include <string>

namespace sp::fmt {

struct FormatOptions {
    int indentWidth{4};
    bool useTabs{false};
    int maxEmptyLines{1};
    bool spaceAroundBinaryOps{true};
    bool spaceAfterComma{true};
    bool spaceAfterColon{true};
};

class Formatter {
public:
    explicit Formatter(FormatOptions options = {});

    std::string format(const std::string& sourceCode);
    bool isFormatted(const std::string& sourceCode);

    const FormatOptions& getOptions() const { return m_options; }
    void setOptions(const FormatOptions& opts) { m_options = opts; }

private:
    FormatOptions m_options;
};

} // namespace sp::fmt
