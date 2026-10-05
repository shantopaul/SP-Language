#pragma once

#include "sp/ast/Declaration.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/sema/SymbolTable.h"
#include "sp/sema/Type.h"
#include <unordered_set>
#include <string>

namespace sp::sema {

class NameResolver {
public:
    NameResolver(TypeContext& typeCtx, SymbolTable& symbolTable, diagnostics::DiagnosticEngine& diagnostics)
        : m_typeCtx(typeCtx), m_symbolTable(symbolTable), m_diagnostics(diagnostics) {}

    void resolve(const ast::Program& program);

private:
    void pass1_collectTopLevel(const ast::Program& program);
    void pass2_resolveMembersAndInheritance(const ast::Program& program);

    void resolveClassDeclPass1(const ast::ClassDecl& decl);
    void resolveInterfaceDeclPass1(const ast::InterfaceDecl& decl);
    void resolveStructDeclPass1(const ast::StructDecl& decl);
    void resolveEnumDeclPass1(const ast::EnumDecl& decl);
    void resolveFunctionDeclPass1(const ast::FunctionDecl& decl);
    void resolveImportDecl(const ast::ImportDecl& decl);

    void resolveClassMembersAndInheritance(const ast::ClassDecl& decl);
    void resolveInterfaceMembers(const ast::InterfaceDecl& decl);
    void resolveStructMembers(const ast::StructDecl& decl);
    void resolveEnumMembers(const ast::EnumDecl& decl);

    bool checkCircularInheritance(const std::shared_ptr<ClassType>& cls, const std::string& superName, const diagnostics::SourceLocation& loc);

    TypePtr resolveTypeWithDiagnostic(const std::string& typeStr, const diagnostics::SourceLocation& loc);

    TypeContext& m_typeCtx;
    SymbolTable& m_symbolTable;
    diagnostics::DiagnosticEngine& m_diagnostics;
    std::unordered_set<std::string> m_importedModules;
    std::vector<std::string> m_activeTypeParams;
};

} // namespace sp::sema
