#include "sp/sema/NameResolver.h"
#include <iostream>

namespace sp::sema {

void NameResolver::resolve(const ast::Program& program) {
    pass1_collectTopLevel(program);
    pass2_resolveMembersAndInheritance(program);
}

void NameResolver::pass1_collectTopLevel(const ast::Program& program) {
    for (const auto& declPtr : program.getDeclarations()) {
        if (!declPtr) continue;

        switch (declPtr->getKind()) {
            case ast::ASTNodeKind::ImportDecl: {
                const auto& importDecl = static_cast<const ast::ImportDecl&>(*declPtr);
                resolveImportDecl(importDecl);
                break;
            }
            case ast::ASTNodeKind::ClassDecl: {
                const auto& classDecl = static_cast<const ast::ClassDecl&>(*declPtr);
                resolveClassDeclPass1(classDecl);
                break;
            }
            case ast::ASTNodeKind::InterfaceDecl: {
                const auto& ifaceDecl = static_cast<const ast::InterfaceDecl&>(*declPtr);
                resolveInterfaceDeclPass1(ifaceDecl);
                break;
            }
            case ast::ASTNodeKind::StructDecl: {
                const auto& structDecl = static_cast<const ast::StructDecl&>(*declPtr);
                resolveStructDeclPass1(structDecl);
                break;
            }
            case ast::ASTNodeKind::EnumDecl: {
                const auto& enumDecl = static_cast<const ast::EnumDecl&>(*declPtr);
                resolveEnumDeclPass1(enumDecl);
                break;
            }
            case ast::ASTNodeKind::FunctionDecl: {
                const auto& fnDecl = static_cast<const ast::FunctionDecl&>(*declPtr);
                resolveFunctionDeclPass1(fnDecl);
                break;
            }
            case ast::ASTNodeKind::BlockStmt: {
                const auto& block = static_cast<const ast::BlockStmt&>(*declPtr);
                for (const auto& inner : block.getStatements()) {
                    if (inner && inner->getKind() == ast::ASTNodeKind::FunctionDecl) {
                        resolveFunctionDeclPass1(static_cast<const ast::FunctionDecl&>(*inner));
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

void NameResolver::resolveImportDecl(const ast::ImportDecl& decl) {
    const std::string& path = decl.getPath();
    if (path.empty()) {
        m_diagnostics.reportError("E1009", "Empty import path specified", decl.getLocation());
        return;
    }
    m_importedModules.insert(path);
}

void NameResolver::resolveClassDeclPass1(const ast::ClassDecl& decl) {
    const std::string& name = decl.getName();
    auto existing = m_symbolTable.lookupCurrentScope(name);
    if (existing) {
        m_diagnostics.reportError("E1004", "Duplicate declaration of class '" + name + "'", decl.getLocation(),
                                  "A symbol with name '" + name + "' is already declared in this scope");
        return;
    }

    auto classType = m_typeCtx.createClassType(name);
    classType->setAbstract(decl.isAbstract());

    auto classSym = std::make_shared<ClassSymbol>(name, classType, decl.getLocation(), decl.getVisibility());
    m_symbolTable.define(classSym);
}

void NameResolver::resolveInterfaceDeclPass1(const ast::InterfaceDecl& decl) {
    const std::string& name = decl.getName();
    auto existing = m_symbolTable.lookupCurrentScope(name);
    if (existing) {
        m_diagnostics.reportError("E1004", "Duplicate declaration of interface '" + name + "'", decl.getLocation(),
                                  "A symbol with name '" + name + "' is already declared in this scope");
        return;
    }

    auto ifaceType = m_typeCtx.createInterfaceType(name);
    auto ifaceSym = std::make_shared<InterfaceSymbol>(name, ifaceType, decl.getLocation(), decl.getVisibility());
    m_symbolTable.define(ifaceSym);
}

void NameResolver::resolveStructDeclPass1(const ast::StructDecl& decl) {
    const std::string& name = decl.getName();
    auto existing = m_symbolTable.lookupCurrentScope(name);
    if (existing) {
        m_diagnostics.reportError("E1004", "Duplicate declaration of struct '" + name + "'", decl.getLocation(),
                                  "A symbol with name '" + name + "' is already declared in this scope");
        return;
    }

    auto structType = m_typeCtx.createStructType(name);
    auto structSym = std::make_shared<StructSymbol>(name, structType, decl.getLocation(), decl.getVisibility());
    m_symbolTable.define(structSym);
}

void NameResolver::resolveEnumDeclPass1(const ast::EnumDecl& decl) {
    const std::string& name = decl.getName();
    auto existing = m_symbolTable.lookupCurrentScope(name);
    if (existing) {
        m_diagnostics.reportError("E1004", "Duplicate declaration of enum '" + name + "'", decl.getLocation(),
                                  "A symbol with name '" + name + "' is already declared in this scope");
        return;
    }

    auto enumType = m_typeCtx.createEnumType(name);
    auto enumSym = std::make_shared<EnumSymbol>(name, enumType, decl.getLocation(), decl.getVisibility());
    m_symbolTable.define(enumSym);
}

void NameResolver::resolveFunctionDeclPass1(const ast::FunctionDecl& decl) {
    const std::string& name = decl.getName();

    std::vector<TypePtr> paramTypes;
    for (const auto& param : decl.getParams()) {
        auto pType = resolveTypeWithDiagnostic(param.getType(), decl.getLocation());
        paramTypes.push_back(pType ? pType : m_typeCtx.getError());
    }

    TypePtr returnType = m_typeCtx.getVoid();
    if (!decl.getReturnType().empty()) {
        auto rType = resolveTypeWithDiagnostic(decl.getReturnType(), decl.getLocation());
        if (rType) returnType = rType;
    }

    auto fnType = m_typeCtx.getFunctionType(std::move(paramTypes), std::move(returnType));
    auto fnSym = std::make_shared<FunctionSymbol>(name, fnType, decl.getLocation(), decl.getVisibility(),
                                                  decl.isStatic(), false, decl.isAbstract(), decl.isOverride());

    if (!m_symbolTable.define(fnSym)) {
        m_diagnostics.reportError("E1004", "Duplicate declaration of function '" + name + "' with identical parameter types",
                                  decl.getLocation(), "Provide unique parameter types or rename the function");
    }
}

void NameResolver::pass2_resolveMembersAndInheritance(const ast::Program& program) {
    for (const auto& declPtr : program.getDeclarations()) {
        if (!declPtr) continue;

        switch (declPtr->getKind()) {
            case ast::ASTNodeKind::ClassDecl: {
                const auto& classDecl = static_cast<const ast::ClassDecl&>(*declPtr);
                resolveClassMembersAndInheritance(classDecl);
                break;
            }
            case ast::ASTNodeKind::InterfaceDecl: {
                const auto& ifaceDecl = static_cast<const ast::InterfaceDecl&>(*declPtr);
                resolveInterfaceMembers(ifaceDecl);
                break;
            }
            case ast::ASTNodeKind::StructDecl: {
                const auto& structDecl = static_cast<const ast::StructDecl&>(*declPtr);
                resolveStructMembers(structDecl);
                break;
            }
            case ast::ASTNodeKind::EnumDecl: {
                const auto& enumDecl = static_cast<const ast::EnumDecl&>(*declPtr);
                resolveEnumMembers(enumDecl);
                break;
            }
            default:
                break;
        }
    }
}

bool NameResolver::checkCircularInheritance(const std::shared_ptr<ClassType>& cls, const std::string& superName, const diagnostics::SourceLocation& loc) {
    auto current = m_typeCtx.findUserType(superName);
    while (current && current->isClass()) {
        auto currentCls = std::static_pointer_cast<ClassType>(current);
        if (currentCls->getName() == cls->getName()) {
            m_diagnostics.reportError("E1004", "Circular inheritance detected: '" + cls->getName() + "' and '" + superName + "' inherit from each other", loc);
            return true;
        }
        current = currentCls->getSuperclass();
    }
    return false;
}

void NameResolver::resolveClassMembersAndInheritance(const ast::ClassDecl& decl) {
    auto userType = m_typeCtx.findUserType(decl.getName());
    if (!userType || !userType->isClass()) return;
    auto clsType = std::static_pointer_cast<ClassType>(userType);

    // Resolve superclass
    if (decl.hasBaseClass()) {
        const std::string& baseName = decl.getBaseClass();
        auto baseSym = m_symbolTable.lookup(baseName);
        if (!baseSym) {
            m_diagnostics.reportError("E1002", "Undefined base class '" + baseName + "'", decl.getLocation());
        } else if (!baseSym->getType() || !baseSym->getType()->isClass()) {
            m_diagnostics.reportError("E1001", "Base type '" + baseName + "' is not a class", decl.getLocation());
        } else {
            if (!checkCircularInheritance(clsType, baseName, decl.getLocation())) {
                clsType->setSuperclass(std::static_pointer_cast<ClassType>(baseSym->getType()));
            }
        }
    }

    // Resolve implemented interfaces
    for (const auto& ifaceName : decl.getInterfaces()) {
        auto ifaceSym = m_symbolTable.lookup(ifaceName);
        if (!ifaceSym) {
            m_diagnostics.reportError("E1002", "Undefined interface '" + ifaceName + "'", decl.getLocation());
        } else if (!ifaceSym->getType() || !ifaceSym->getType()->isInterface()) {
            m_diagnostics.reportError("E1001", "Type '" + ifaceName + "' is not an interface", decl.getLocation());
        } else {
            clsType->addInterface(std::static_pointer_cast<InterfaceType>(ifaceSym->getType()));
        }
    }

    // Resolve members (fields and methods)
    for (const auto& memberNode : decl.getMembers()) {
        if (!memberNode) continue;

        if (memberNode->getKind() == ast::ASTNodeKind::VarDeclStmt) {
            const auto& varDecl = static_cast<const ast::VarDeclStmt&>(*memberNode);
            const std::string& fieldName = varDecl.getName();

            if (clsType->findField(fieldName).has_value()) {
                m_diagnostics.reportError("E1004", "Duplicate field '" + fieldName + "' in class '" + decl.getName() + "'", varDecl.getLocation());
                continue;
            }

            auto fieldType = resolveTypeWithDiagnostic(varDecl.getType(), varDecl.getLocation());
            if (!fieldType) fieldType = m_typeCtx.getError();

            MemberField field{
                .name = fieldName,
                .type = fieldType,
                .visibility = "public",
                .isStatic = false,
                .isConst = varDecl.isConst()
            };
            clsType->addField(std::move(field));
        } else if (memberNode->getKind() == ast::ASTNodeKind::FunctionDecl) {
            const auto& fnDecl = static_cast<const ast::FunctionDecl&>(*memberNode);
            const std::string& methodName = fnDecl.getName();

            std::vector<TypePtr> paramTypes;
            for (const auto& param : fnDecl.getParams()) {
                auto pType = resolveTypeWithDiagnostic(param.getType(), fnDecl.getLocation());
                paramTypes.push_back(pType ? pType : m_typeCtx.getError());
            }

            TypePtr returnType = m_typeCtx.getVoid();
            if (!fnDecl.getReturnType().empty()) {
                auto rType = resolveTypeWithDiagnostic(fnDecl.getReturnType(), fnDecl.getLocation());
                if (rType) returnType = rType;
            }

            auto fnType = m_typeCtx.getFunctionType(std::move(paramTypes), std::move(returnType));

            // Check duplicate method in current class
            bool duplicate = false;
            for (const auto& em : clsType->getMethods()) {
                if (em.name == methodName && em.functionType->equals(*fnType)) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) {
                m_diagnostics.reportError("E1004", "Duplicate method '" + methodName + "' in class '" + decl.getName() + "' with identical signature", fnDecl.getLocation());
                continue;
            }

            // Check override validity
            if (fnDecl.isOverride()) {
                bool foundSuperMethod = false;
                if (clsType->getSuperclass()) {
                    auto superMethods = clsType->getSuperclass()->findMethods(methodName);
                    for (const auto& sm : superMethods) {
                        if (sm.functionType->equals(*fnType)) {
                            foundSuperMethod = true;
                            break;
                        }
                    }
                }
                if (!foundSuperMethod) {
                    m_diagnostics.reportError("E1001", "Method '" + methodName + "' marked override does not override any base class method", fnDecl.getLocation());
                }
            }

            MemberMethod method{
                .name = methodName,
                .functionType = fnType,
                .visibility = fnDecl.getVisibility().empty() ? "public" : fnDecl.getVisibility(),
                .isStatic = fnDecl.isStatic(),
                .isAbstract = fnDecl.isAbstract(),
                .isOverride = fnDecl.isOverride()
            };
            clsType->addMethod(std::move(method));
        }
    }
}

void NameResolver::resolveInterfaceMembers(const ast::InterfaceDecl& decl) {
    auto userType = m_typeCtx.findUserType(decl.getName());
    if (!userType || !userType->isInterface()) return;
    auto ifaceType = std::static_pointer_cast<InterfaceType>(userType);

    for (const auto& methodPtr : decl.getMethods()) {
        if (!methodPtr) continue;
        const auto& fnDecl = *methodPtr;
        const std::string& methodName = fnDecl.getName();

        std::vector<TypePtr> paramTypes;
        for (const auto& param : fnDecl.getParams()) {
            auto pType = resolveTypeWithDiagnostic(param.getType(), fnDecl.getLocation());
            paramTypes.push_back(pType ? pType : m_typeCtx.getError());
        }

        TypePtr returnType = m_typeCtx.getVoid();
        if (!fnDecl.getReturnType().empty()) {
            auto rType = resolveTypeWithDiagnostic(fnDecl.getReturnType(), fnDecl.getLocation());
            if (rType) returnType = rType;
        }

        auto fnType = m_typeCtx.getFunctionType(std::move(paramTypes), std::move(returnType));

        MemberMethod method{
            .name = methodName,
            .functionType = fnType,
            .visibility = "public",
            .isStatic = false,
            .isAbstract = true,
            .isOverride = false
        };
        ifaceType->addMethod(std::move(method));
    }
}

void NameResolver::resolveStructMembers(const ast::StructDecl& decl) {
    auto userType = m_typeCtx.findUserType(decl.getName());
    if (!userType || !userType->isStruct()) return;
    auto structType = std::static_pointer_cast<StructType>(userType);

    for (const auto& fieldPtr : decl.getFields()) {
        if (!fieldPtr) continue;
        const auto& field = *fieldPtr;
        const std::string& fieldName = field.getName();

        if (structType->findField(fieldName).has_value()) {
            m_diagnostics.reportError("E1004", "Duplicate field '" + fieldName + "' in struct '" + decl.getName() + "'", field.getLocation());
            continue;
        }

        auto fType = resolveTypeWithDiagnostic(field.getType(), field.getLocation());
        if (!fType) fType = m_typeCtx.getError();

        MemberField mf{
            .name = fieldName,
            .type = fType,
            .visibility = "public",
            .isStatic = false,
            .isConst = field.isConst()
        };
        structType->addField(std::move(mf));
    }
}

void NameResolver::resolveEnumMembers(const ast::EnumDecl& decl) {
    auto userType = m_typeCtx.findUserType(decl.getName());
    if (!userType || !userType->isEnum()) return;
    auto enumType = std::static_pointer_cast<EnumType>(userType);

    for (const auto& variant : decl.getVariants()) {
        if (enumType->hasVariant(variant.getName())) {
            m_diagnostics.reportError("E1004", "Duplicate enum variant '" + variant.getName() + "' in enum '" + decl.getName() + "'", decl.getLocation());
            continue;
        }
        enumType->addVariant(variant.getName());
    }
}

TypePtr NameResolver::resolveTypeWithDiagnostic(const std::string& typeStr, const diagnostics::SourceLocation& loc) {
    if (typeStr.empty()) return m_typeCtx.getVoid();
    auto type = m_typeCtx.resolveType(typeStr);
    if (!type) {
        m_diagnostics.reportError("E1002", "Undefined type '" + typeStr + "'", loc,
                                  "Check the type name or make sure the class/struct/enum is defined");
        return m_typeCtx.getError();
    }
    return type;
}

} // namespace sp::sema
