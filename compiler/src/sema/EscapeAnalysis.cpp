#include "sp/sema/EscapeAnalysis.h"
#include "sp/sema/TypeChecker.h"
#include <queue>

namespace sp::sema {

// ----------------------------------------------------------------------------
// FunctionEscapeSummary Implementation
// ----------------------------------------------------------------------------

void FunctionEscapeSummary::recordVariable(const std::string& name, bool isRef) {
    if (m_variables.find(name) == m_variables.end()) {
        m_variables[name] = VarEscapeInfo{name, EscapeState::LocalNonEscaping, isRef, {}};
    }
}

void FunctionEscapeSummary::markEscaping(const std::string& name, const std::string& reason) {
    auto it = m_variables.find(name);
    if (it != m_variables.end()) {
        it->second.state = EscapeState::GlobalEscaping;
        it->second.escapeReasons.push_back(reason);
    }
}

bool FunctionEscapeSummary::escapes(const std::string& name) const {
    auto it = m_variables.find(name);
    return it != m_variables.end() && it->second.state == EscapeState::GlobalEscaping;
}

EscapeState FunctionEscapeSummary::getEscapeState(const std::string& name) const {
    auto it = m_variables.find(name);
    if (it != m_variables.end()) {
        return it->second.state;
    }
    return EscapeState::LocalNonEscaping;
}

const VarEscapeInfo* FunctionEscapeSummary::getVariableInfo(const std::string& name) const {
    auto it = m_variables.find(name);
    if (it != m_variables.end()) {
        return &it->second;
    }
    return nullptr;
}

// ----------------------------------------------------------------------------
// EscapeAnalysis Implementation
// ----------------------------------------------------------------------------

EscapeAnalysis::EscapeAnalysis(const TypeChecker* typeChecker)
    : m_typeChecker(typeChecker) {}

void EscapeAnalysis::analyze(const ast::Program& program) {
    m_summaries.clear();
    program.accept(*this);
}

void EscapeAnalysis::analyzeFunction(const ast::FunctionDecl& fnDecl) {
    std::string fnName = fnDecl.getName();
    FunctionEscapeSummary summary(fnName);
    m_currentSummary = &summary;
    m_aliasGraph.clear();

    if (fnDecl.hasBody()) {
        fnDecl.getBody()->accept(*this);
    } else if (fnDecl.isShortForm() && fnDecl.getShortBody()) {
        std::vector<std::string> returnedIds;
        collectIdentifiers(fnDecl.getShortBody(), returnedIds);
        for (const auto& id : returnedIds) {
            summary.markEscaping(id, "returned from expression body");
        }
        fnDecl.getShortBody()->accept(*this);
    }

    propagateAliases();

    m_summaries[fnName] = std::move(summary);
    m_currentSummary = nullptr;
}

const FunctionEscapeSummary* EscapeAnalysis::getSummary(const std::string& funcName) const {
    auto it = m_summaries.find(funcName);
    if (it != m_summaries.end()) {
        return &it->second;
    }
    return nullptr;
}

bool EscapeAnalysis::escapes(const std::string& funcName, const std::string& varName) const {
    const auto* summary = getSummary(funcName);
    return summary && summary->escapes(varName);
}

EscapeState EscapeAnalysis::getEscapeState(const std::string& funcName, const std::string& varName) const {
    const auto* summary = getSummary(funcName);
    if (summary) {
        return summary->getEscapeState(varName);
    }
    return EscapeState::LocalNonEscaping;
}

void EscapeAnalysis::collectIdentifiers(const ast::ExpressionNode* expr, std::vector<std::string>& outNames) {
    if (!expr) return;

    switch (expr->getKind()) {
        case ast::ASTNodeKind::IdentifierExpr: {
            const auto* id = static_cast<const ast::IdentifierExpr*>(expr);
            outNames.push_back(id->getName());
            break;
        }
        case ast::ASTNodeKind::BinaryExpr: {
            const auto* bin = static_cast<const ast::BinaryExpr*>(expr);
            collectIdentifiers(&bin->getLeft(), outNames);
            collectIdentifiers(&bin->getRight(), outNames);
            break;
        }
        case ast::ASTNodeKind::UnaryExpr: {
            const auto* un = static_cast<const ast::UnaryExpr*>(expr);
            collectIdentifiers(&un->getOperand(), outNames);
            break;
        }
        case ast::ASTNodeKind::TryExpr: {
            const auto* tr = static_cast<const ast::TryExpr*>(expr);
            collectIdentifiers(&tr->getExpr(), outNames);
            break;
        }
        case ast::ASTNodeKind::TernaryExpr: {
            const auto* tern = static_cast<const ast::TernaryExpr*>(expr);
            collectIdentifiers(&tern->getCondition(), outNames);
            collectIdentifiers(&tern->getThenExpr(), outNames);
            collectIdentifiers(&tern->getElseExpr(), outNames);
            break;
        }
        case ast::ASTNodeKind::MemberExpr: {
            const auto* mem = static_cast<const ast::MemberExpr*>(expr);
            collectIdentifiers(&mem->getObject(), outNames);
            break;
        }
        case ast::ASTNodeKind::IndexExpr: {
            const auto* idx = static_cast<const ast::IndexExpr*>(expr);
            collectIdentifiers(&idx->getObject(), outNames);
            collectIdentifiers(&idx->getIndex(), outNames);
            break;
        }
        case ast::ASTNodeKind::ArrayExpr: {
            const auto* arr = static_cast<const ast::ArrayExpr*>(expr);
            for (const auto& elem : arr->getElements()) {
                collectIdentifiers(elem.get(), outNames);
            }
            break;
        }
        case ast::ASTNodeKind::MapExpr: {
            const auto* mapNode = static_cast<const ast::MapExpr*>(expr);
            for (const auto& entry : mapNode->getEntries()) {
                collectIdentifiers(entry.first.get(), outNames);
                collectIdentifiers(entry.second.get(), outNames);
            }
            break;
        }
        case ast::ASTNodeKind::CallExpr: {
            const auto* call = static_cast<const ast::CallExpr*>(expr);
            collectIdentifiers(&call->getCallee(), outNames);
            for (const auto& arg : call->getArgs()) {
                collectIdentifiers(arg.get(), outNames);
            }
            break;
        }
        case ast::ASTNodeKind::NewExpr: {
            const auto* newExpr = static_cast<const ast::NewExpr*>(expr);
            for (const auto& arg : newExpr->getArgs()) {
                collectIdentifiers(arg.get(), outNames);
            }
            break;
        }
        default:
            break;
    }
}

void EscapeAnalysis::propagateAliases() {
    if (!m_currentSummary) return;

    std::queue<std::string> escapingWorklist;
    for (const auto& [name, info] : m_currentSummary->getVariables()) {
        if (info.state == EscapeState::GlobalEscaping) {
            escapingWorklist.push(name);
        }
    }

    while (!escapingWorklist.empty()) {
        std::string currentEscaping = escapingWorklist.front();
        escapingWorklist.pop();

        auto it = m_aliasGraph.find(currentEscaping);
        if (it != m_aliasGraph.end()) {
            for (const auto& sourceVar : it->second) {
                if (m_currentSummary->getEscapeState(sourceVar) != EscapeState::GlobalEscaping) {
                    m_currentSummary->markEscaping(sourceVar, "aliased to escaping variable " + currentEscaping);
                    escapingWorklist.push(sourceVar);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// AST Visitor Implementation
// ----------------------------------------------------------------------------

void EscapeAnalysis::visit(const ast::LiteralExpr&) {}

void EscapeAnalysis::visit(const ast::IdentifierExpr&) {}

void EscapeAnalysis::visit(const ast::BinaryExpr& node) {
    node.getLeft().accept(*this);
    node.getRight().accept(*this);
}

void EscapeAnalysis::visit(const ast::UnaryExpr& node) {
    node.getOperand().accept(*this);
}

void EscapeAnalysis::visit(const ast::CallExpr& node) {
    node.getCallee().accept(*this);
    for (const auto& arg : node.getArgs()) {
        if (arg) arg->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::MemberExpr& node) {
    node.getObject().accept(*this);
}

void EscapeAnalysis::visit(const ast::IndexExpr& node) {
    node.getObject().accept(*this);
    node.getIndex().accept(*this);
}

void EscapeAnalysis::visit(const ast::ArrayExpr& node) {
    for (const auto& elem : node.getElements()) {
        if (elem) elem->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::MapExpr& node) {
    for (const auto& entry : node.getEntries()) {
        if (entry.first) entry.first->accept(*this);
        if (entry.second) entry.second->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::LambdaExpr& node) {
    node.getBody().accept(*this);
}

void EscapeAnalysis::visit(const ast::TernaryExpr& node) {
    node.getCondition().accept(*this);
    node.getThenExpr().accept(*this);
    node.getElseExpr().accept(*this);
}

void EscapeAnalysis::visit(const ast::AssignExpr& node) {
    if (!m_currentSummary) return;

    const auto& lhs = node.getTarget();
    const auto& rhs = node.getValue();

    std::vector<std::string> rhsVars;
    collectIdentifiers(&rhs, rhsVars);

    if (lhs.getKind() == ast::ASTNodeKind::MemberExpr) {
        for (const auto& var : rhsVars) {
            m_currentSummary->markEscaping(var, "stored into object member field");
        }
    } else if (lhs.getKind() == ast::ASTNodeKind::IndexExpr) {
        for (const auto& var : rhsVars) {
            m_currentSummary->markEscaping(var, "stored into collection index");
        }
    } else if (lhs.getKind() == ast::ASTNodeKind::IdentifierExpr) {
        const auto& id = static_cast<const ast::IdentifierExpr&>(lhs);
        std::string lhsName = id.getName();

        const auto& locals = m_currentSummary->getVariables();
        if (locals.find(lhsName) == locals.end()) {
            for (const auto& var : rhsVars) {
                m_currentSummary->markEscaping(var, "stored to outer scope or global variable " + lhsName);
            }
        } else {
            for (const auto& var : rhsVars) {
                m_aliasGraph[lhsName].insert(var);
            }
        }
    }

    lhs.accept(*this);
    rhs.accept(*this);
}

void EscapeAnalysis::visit(const ast::NewExpr& node) {
    for (const auto& arg : node.getArgs()) {
        if (arg) arg->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::TryExpr& node) {
    node.getExpr().accept(*this);
}

void EscapeAnalysis::visit(const ast::AwaitExpr& node) {
    node.getExpr().accept(*this);
}

void EscapeAnalysis::visit(const ast::ComptimeExpr& node) {
    if (node.hasExpr()) {
        node.getExpr()->accept(*this);
    } else if (node.isBlock() && node.getBlock()) {
        node.getBlock()->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::BlockStmt& node) {
    for (const auto& stmt : node.getStatements()) {
        if (stmt) stmt->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::ExprStmt& node) {
    node.getExpr().accept(*this);
}

void EscapeAnalysis::visit(const ast::VarDeclStmt& node) {
    if (m_currentSummary) {
        bool isRef = false;
        if (m_typeChecker && node.getInitializer()) {
            auto type = m_typeChecker->getNodeType(node.getInitializer());
            if (type) {
                isRef = type->isReferenceType();
            }
        } else if (!node.getType().empty()) {
            const auto& t = node.getType();
            if (t != "int" && t != "float" && t != "double" && t != "bool" &&
                t != "char" && t != "void" && t != "int8" && t != "uint8" &&
                t != "int16" && t != "uint16" && t != "int32" && t != "uint32" &&
                t != "int64" && t != "uint64") {
                isRef = true;
            }
        }

        m_currentSummary->recordVariable(node.getName(), isRef);

        if (node.hasInitializer()) {
            std::vector<std::string> rhsVars;
            collectIdentifiers(node.getInitializer(), rhsVars);
            for (const auto& var : rhsVars) {
                m_aliasGraph[node.getName()].insert(var);
            }
        }
    }

    if (node.hasInitializer()) {
        node.getInitializer()->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::IfStmt& node) {
    node.getCondition().accept(*this);
    node.getThenBranch().accept(*this);
    if (node.hasElseBranch() && node.getElseBranch()) {
        node.getElseBranch()->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::WhileStmt& node) {
    node.getCondition().accept(*this);
    node.getBody().accept(*this);
}

void EscapeAnalysis::visit(const ast::DoWhileStmt& node) {
    node.getBody().accept(*this);
    node.getCondition().accept(*this);
}

void EscapeAnalysis::visit(const ast::ForStmt& node) {
    if (node.hasInit()) node.getInit()->accept(*this);
    if (node.hasCondition()) node.getCondition()->accept(*this);
    if (node.hasIncrement()) node.getIncrement()->accept(*this);
    node.getBody().accept(*this);
}

void EscapeAnalysis::visit(const ast::ForInStmt& node) {
    node.getIterable().accept(*this);
    node.getBody().accept(*this);
}

void EscapeAnalysis::visit(const ast::SwitchStmt& node) {
    node.getSubject().accept(*this);
    for (const auto& c : node.getCases()) {
        for (const auto& stmt : c.getStatements()) {
            if (stmt) stmt->accept(*this);
        }
    }
    for (const auto& stmt : node.getDefaultStatements()) {
        if (stmt) stmt->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::MatchStmt& node) {
    node.getSubject().accept(*this);
    for (const auto& arm : node.getArms()) {
        arm.getBody().accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::ReturnStmt& node) {
    if (m_currentSummary && node.hasValue()) {
        std::vector<std::string> returnedVars;
        collectIdentifiers(node.getValue(), returnedVars);
        for (const auto& var : returnedVars) {
            m_currentSummary->markEscaping(var, "returned from function");
        }
    }

    if (node.hasValue() && node.getValue()) {
        node.getValue()->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::BreakStmt&) {}

void EscapeAnalysis::visit(const ast::ContinueStmt&) {}

void EscapeAnalysis::visit(const ast::TryCatchStmt& node) {
    node.getTryBlock().accept(*this);
    for (const auto& c : node.getCatches()) {
        c.getBody().accept(*this);
    }
    if (node.hasFinally() && node.getFinallyBlock()) {
        node.getFinallyBlock()->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::ThrowStmt& node) {
    node.getExpr().accept(*this);
}

void EscapeAnalysis::visit(const ast::FunctionDecl& node) {
    analyzeFunction(node);
}

void EscapeAnalysis::visit(const ast::ClassDecl& node) {
    for (const auto& member : node.getMembers()) {
        if (member) member->accept(*this);
    }
}

void EscapeAnalysis::visit(const ast::InterfaceDecl&) {}

void EscapeAnalysis::visit(const ast::StructDecl&) {}

void EscapeAnalysis::visit(const ast::EnumDecl&) {}

void EscapeAnalysis::visit(const ast::ModuleDecl&) {}

void EscapeAnalysis::visit(const ast::ImportDecl&) {}

void EscapeAnalysis::visit(const ast::Program& node) {
    for (const auto& decl : node.getDeclarations()) {
        if (decl) decl->accept(*this);
    }
}

} // namespace sp::sema
