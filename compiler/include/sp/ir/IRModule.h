#pragma once

#include "sp/ir/IRFunction.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace sp::ir {

struct StringLiteralInfo {
    std::string name;     // e.g. "@.str.0"
    std::string content;  // e.g. "Hello, SP!"
};

class IRModule {
public:
    explicit IRModule(std::string name) : m_name(std::move(name)) {
        initRuntimeDeclarations();
    }

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const std::vector<IRFunctionPtr>& getFunctions() const noexcept { return m_functions; }
    [[nodiscard]] std::vector<IRFunctionPtr>& getFunctions() noexcept { return m_functions; }

    [[nodiscard]] const std::vector<StringLiteralInfo>& getStringLiterals() const noexcept { return m_stringLiterals; }

    IRFunction* createFunction(const std::string& name,
                               IRTypePtr returnType,
                               std::vector<std::shared_ptr<ParameterValue>> params,
                               bool isExternal = false) {
        auto func = std::make_unique<IRFunction>(name, std::move(returnType), std::move(params), isExternal, this);
        auto* rawPtr = func.get();
        m_functions.push_back(std::move(func));
        m_functionMap[name] = rawPtr;
        return rawPtr;
    }

    [[nodiscard]] IRFunction* getFunction(const std::string& name) const {
        auto it = m_functionMap.find(name);
        if (it != m_functionMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Allocate or retrieve deduplicated global string literal
    std::shared_ptr<ConstantString> getOrCreateStringLiteral(const std::string& str) {
        auto it = m_stringLiteralMap.find(str);
        if (it != m_stringLiteralMap.end()) {
            return std::make_shared<ConstantString>(str, it->second);
        }

        std::string globalName = "@.str." + std::to_string(m_stringLiterals.size());
        m_stringLiterals.push_back({globalName, str});
        m_stringLiteralMap[str] = globalName;
        return std::make_shared<ConstantString>(str, globalName);
    }

    void initRuntimeDeclarations();

private:
    std::string m_name;
    std::vector<IRFunctionPtr> m_functions;
    std::unordered_map<std::string, IRFunction*> m_functionMap;
    std::vector<StringLiteralInfo> m_stringLiterals;
    std::unordered_map<std::string, std::string> m_stringLiteralMap;
};

using IRModulePtr = std::unique_ptr<IRModule>;

} // namespace sp::ir
