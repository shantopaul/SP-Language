#include "sp/ast/ASTPrinter.h"
#include "sp/ast/Expression.h"
#include "sp/ast/Statement.h"
#include "sp/ast/Declaration.h"
#include "sp/lexer/TokenType.h"

namespace sp::ast {

std::string ASTPrinter::print(const ASTNode& node) {
    m_out.str("");
    m_out.clear();
    m_indent = 0;
    node.accept(*this);
    return m_out.str();
}

void ASTPrinter::indent() {
    for (int i = 0; i < m_indent; ++i) {
        m_out << "  ";
    }
}

void ASTPrinter::line(const std::string& text) {
    indent();
    m_out << text << "\n";
}

void ASTPrinter::visit(const LiteralExpr& node) {
    line("LiteralExpr(" + node.getValue() + ")");
}

void ASTPrinter::visit(const IdentifierExpr& node) {
    line("IdentifierExpr(" + node.getName() + ")");
}

void ASTPrinter::visit(const BinaryExpr& node) {
    line("BinaryExpr(" + std::string(lexer::tokenTypeToString(node.getOp())) + ")");
    m_indent++;
    node.getLeft().accept(*this);
    node.getRight().accept(*this);
    m_indent--;
}

void ASTPrinter::visit(const UnaryExpr& node) {
    line("UnaryExpr(" + std::string(lexer::tokenTypeToString(node.getOp())) + ", prefix=" + (node.isPrefix() ? "true" : "false") + ")");
    m_indent++;
    node.getOperand().accept(*this);
    m_indent--;
}

void ASTPrinter::visit(const CallExpr& node) {
    line("CallExpr:");
    m_indent++;
    line("Callee:");
    m_indent++;
    node.getCallee().accept(*this);
    m_indent--;
    line("Args (" + std::to_string(node.getArgs().size()) + "):");
    m_indent++;
    for (const auto& arg : node.getArgs()) {
        arg->accept(*this);
    }
    m_indent--;
    m_indent--;
}

void ASTPrinter::visit(const MemberExpr& node) {
    line("MemberExpr(." + node.getMember() + ", safeNav=" + (node.isSafeNav() ? "true" : "false") + "):");
    m_indent++;
    node.getObject().accept(*this);
    m_indent--;
}

void ASTPrinter::visit(const IndexExpr& node) {
    line("IndexExpr:");
    m_indent++;
    node.getObject().accept(*this);
    node.getIndex().accept(*this);
    m_indent--;
}

void ASTPrinter::visit(const ArrayExpr& node) {
    line("ArrayExpr (" + std::to_string(node.getElements().size()) + " elements):");
    m_indent++;
    for (const auto& el : node.getElements()) {
        el->accept(*this);
    }
    m_indent--;
}

void ASTPrinter::visit(const MapExpr& node) {
    line("MapExpr (" + std::to_string(node.getEntries().size()) + " entries):");
    m_indent++;
    for (const auto& entry : node.getEntries()) {
        line("Key:");
        m_indent++;
        entry.first->accept(*this);
        m_indent--;
        line("Value:");
        m_indent++;
        entry.second->accept(*this);
        m_indent--;
    }
    m_indent--;
}

void ASTPrinter::visit(const LambdaExpr& node) {
    std::string paramsStr;
    for (size_t i = 0; i < node.getParams().size(); ++i) {
        if (i > 0) paramsStr += ", ";
        paramsStr += node.getParams()[i].type + " " + node.getParams()[i].name;
    }
    line("LambdaExpr(" + paramsStr + ") -> " + (node.getReturnType().empty() ? "auto" : node.getReturnType()) + ":");
    m_indent++;
    node.getBody().accept(*this);
    m_indent--;
}

void ASTPrinter::visit(const TernaryExpr& node) {
    line("TernaryExpr:");
    m_indent++;
    line("Condition:");
    m_indent++;
    node.getCondition().accept(*this);
    m_indent--;
    line("Then:");
    m_indent++;
    node.getThenExpr().accept(*this);
    m_indent--;
    line("Else:");
    m_indent++;
    node.getElseExpr().accept(*this);
    m_indent--;
    m_indent--;
}

void ASTPrinter::visit(const AssignExpr& node) {
    line("AssignExpr(" + std::string(lexer::tokenTypeToString(node.getOp())) + "):");
    m_indent++;
    node.getTarget().accept(*this);
    node.getValue().accept(*this);
    m_indent--;
}

void ASTPrinter::visit(const NewExpr& node) {
    line("NewExpr(" + node.getTypeName() + "):");
    m_indent++;
    for (const auto& arg : node.getArgs()) {
        arg->accept(*this);
    }
    m_indent--;
}

void ASTPrinter::visit(const BlockStmt& node) {
    line("BlockStmt (" + std::to_string(node.getStatements().size()) + " statements):");
    m_indent++;
    for (const auto& stmt : node.getStatements()) {
        stmt->accept(*this);
    }
    m_indent--;
}

void ASTPrinter::visit(const ExprStmt& node) {
    line("ExprStmt:");
    m_indent++;
    node.getExpr().accept(*this);
    m_indent--;
}

void ASTPrinter::visit(const VarDeclStmt& node) {
    std::string kind = node.isConst() ? "const " : (node.isVar() ? "var " : "");
    line("VarDeclStmt(" + kind + node.getType() + " " + node.getName() + "):");
    if (node.hasInitializer()) {
        m_indent++;
        node.getInitializer()->accept(*this);
        m_indent--;
    }
}

void ASTPrinter::visit(const IfStmt& node) {
    line("IfStmt:");
    m_indent++;
    line("Condition:");
    m_indent++;
    node.getCondition().accept(*this);
    m_indent--;
    line("Then:");
    m_indent++;
    node.getThenBranch().accept(*this);
    m_indent--;
    if (node.hasElseBranch()) {
        line("Else:");
        m_indent++;
        node.getElseBranch()->accept(*this);
        m_indent--;
    }
    m_indent--;
}

void ASTPrinter::visit(const WhileStmt& node) {
    line("WhileStmt:");
    m_indent++;
    line("Condition:");
    m_indent++;
    node.getCondition().accept(*this);
    m_indent--;
    line("Body:");
    m_indent++;
    node.getBody().accept(*this);
    m_indent--;
    m_indent--;
}

void ASTPrinter::visit(const DoWhileStmt& node) {
    line("DoWhileStmt:");
    m_indent++;
    line("Body:");
    m_indent++;
    node.getBody().accept(*this);
    m_indent--;
    line("Condition:");
    m_indent++;
    node.getCondition().accept(*this);
    m_indent--;
    m_indent--;
}

void ASTPrinter::visit(const ForStmt& node) {
    line("ForStmt:");
    m_indent++;
    if (node.hasInit()) {
        line("Init:");
        m_indent++;
        node.getInit()->accept(*this);
        m_indent--;
    }
    if (node.hasCondition()) {
        line("Condition:");
        m_indent++;
        node.getCondition()->accept(*this);
        m_indent--;
    }
    if (node.hasIncrement()) {
        line("Increment:");
        m_indent++;
        node.getIncrement()->accept(*this);
        m_indent--;
    }
    line("Body:");
    m_indent++;
    node.getBody().accept(*this);
    m_indent--;
    m_indent--;
}

void ASTPrinter::visit(const ForInStmt& node) {
    line("ForInStmt(" + node.getType() + " " + node.getVarName() + " in ...):");
    m_indent++;
    line("Iterable:");
    m_indent++;
    node.getIterable().accept(*this);
    m_indent--;
    line("Body:");
    m_indent++;
    node.getBody().accept(*this);
    m_indent--;
    m_indent--;
}

void ASTPrinter::visit(const SwitchStmt& node) {
    line("SwitchStmt:");
    m_indent++;
    line("Subject:");
    m_indent++;
    node.getSubject().accept(*this);
    m_indent--;
    line("Cases (" + std::to_string(node.getCases().size()) + "):");
    m_indent++;
    for (const auto& c : node.getCases()) {
        line("Case:");
        m_indent++;
        c.getValue().accept(*this);
        for (const auto& stmt : c.getStatements()) {
            stmt->accept(*this);
        }
        m_indent--;
    }
    m_indent--;
    if (!node.getDefaultStatements().empty()) {
        line("Default:");
        m_indent++;
        for (const auto& stmt : node.getDefaultStatements()) {
            stmt->accept(*this);
        }
        m_indent--;
    }
    m_indent--;
}

void ASTPrinter::visit(const MatchStmt& node) {
    line("MatchStmt:");
    m_indent++;
    line("Subject:");
    m_indent++;
    node.getSubject().accept(*this);
    m_indent--;
    line("Arms (" + std::to_string(node.getArms().size()) + "):");
    m_indent++;
    for (const auto& arm : node.getArms()) {
        if (arm.isWildcard()) {
            line("Arm(_):");
        } else {
            line("Arm:");
            m_indent++;
            arm.getPattern()->accept(*this);
            m_indent--;
        }
        m_indent++;
        arm.getBody().accept(*this);
        m_indent--;
    }
    m_indent--;
    m_indent--;
}

void ASTPrinter::visit(const ReturnStmt& node) {
    if (node.hasValue()) {
        line("ReturnStmt:");
        m_indent++;
        node.getValue()->accept(*this);
        m_indent--;
    } else {
        line("ReturnStmt()");
    }
}

void ASTPrinter::visit(const BreakStmt&) {
    line("BreakStmt");
}

void ASTPrinter::visit(const ContinueStmt&) {
    line("ContinueStmt");
}

void ASTPrinter::visit(const TryCatchStmt& node) {
    line("TryCatchStmt:");
    m_indent++;
    line("Try:");
    m_indent++;
    node.getTryBlock().accept(*this);
    m_indent--;
    for (const auto& c : node.getCatches()) {
        line("Catch(" + c.getType() + " " + c.getName() + "):");
        m_indent++;
        c.getBody().accept(*this);
        m_indent--;
    }
    if (node.hasFinally()) {
        line("Finally:");
        m_indent++;
        node.getFinallyBlock()->accept(*this);
        m_indent--;
    }
    m_indent--;
}

void ASTPrinter::visit(const ThrowStmt& node) {
    line("ThrowStmt:");
    m_indent++;
    node.getExpr().accept(*this);
    m_indent--;
}

void ASTPrinter::visit(const FunctionDecl& node) {
    std::string flags;
    if (!node.getVisibility().empty()) flags += node.getVisibility() + " ";
    if (node.isStatic()) flags += "static ";
    if (node.isOverride()) flags += "override ";
    if (node.isAbstract()) flags += "abstract ";
    if (node.isAsync()) flags += "async ";

    std::string paramsStr;
    for (size_t i = 0; i < node.getParams().size(); ++i) {
        if (i > 0) paramsStr += ", ";
        paramsStr += node.getParams()[i].getType() + " " + node.getParams()[i].getName();
    }

    line("FunctionDecl(" + flags + node.getName() + "(" + paramsStr + ") -> " + (node.getReturnType().empty() ? "void" : node.getReturnType()) + "):");
    m_indent++;
    if (node.isShortForm()) {
        line("Body(=>):");
        m_indent++;
        node.getShortBody()->accept(*this);
        m_indent--;
    } else if (node.hasBody()) {
        node.getBody()->accept(*this);
    }
    m_indent--;
}

void ASTPrinter::visit(const ClassDecl& node) {
    std::string header = (node.getVisibility().empty() ? "" : node.getVisibility() + " ") +
                         (node.isAbstract() ? "abstract " : "") +
                         "class " + node.getName();
    if (node.hasBaseClass()) {
        header += " extends " + node.getBaseClass();
    }
    line("ClassDecl(" + header + "):");
    m_indent++;
    for (const auto& member : node.getMembers()) {
        member->accept(*this);
    }
    m_indent--;
}

void ASTPrinter::visit(const InterfaceDecl& node) {
    line("InterfaceDecl(" + node.getName() + "):");
    m_indent++;
    for (const auto& method : node.getMethods()) {
        method->accept(*this);
    }
    m_indent--;
}

void ASTPrinter::visit(const StructDecl& node) {
    line("StructDecl(" + node.getName() + "):");
    m_indent++;
    for (const auto& field : node.getFields()) {
        field->accept(*this);
    }
    m_indent--;
}

void ASTPrinter::visit(const EnumDecl& node) {
    line("EnumDecl(" + node.getName() + "):");
    m_indent++;
    for (const auto& variant : node.getVariants()) {
        line("Variant(" + variant.getName() + ")");
    }
    m_indent--;
}

void ASTPrinter::visit(const ModuleDecl& node) {
    line("ModuleDecl(" + node.getName() + ")");
}

void ASTPrinter::visit(const ImportDecl& node) {
    line("ImportDecl(" + node.getPath() + (node.isWildcard() ? ".*" : "") + ")");
}

void ASTPrinter::visit(const Program& node) {
    line("Program (" + std::to_string(node.getDeclarations().size()) + " declarations):");
    m_indent++;
    for (const auto& decl : node.getDeclarations()) {
        decl->accept(*this);
    }
    m_indent--;
}

} // namespace sp::ast
