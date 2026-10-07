#include "sp/lexer/TokenType.h"

namespace sp::lexer {

std::string_view tokenTypeToString(TokenType type) noexcept {
    switch (type) {
        case TokenType::EndOfFile: return "EndOfFile";
        case TokenType::Invalid: return "Invalid";

        case TokenType::IntegerLiteral: return "IntegerLiteral";
        case TokenType::FloatLiteral: return "FloatLiteral";
        case TokenType::StringLiteral: return "StringLiteral";
        case TokenType::CharLiteral: return "CharLiteral";
        case TokenType::BooleanLiteral: return "BooleanLiteral";
        case TokenType::Identifier: return "Identifier";

        case TokenType::KeywordFn: return "fn";
        case TokenType::KeywordClass: return "class";
        case TokenType::KeywordInterface: return "interface";
        case TokenType::KeywordStruct: return "struct";
        case TokenType::KeywordEnum: return "enum";
        case TokenType::KeywordTrait: return "trait";

        case TokenType::KeywordIf: return "if";
        case TokenType::KeywordElse: return "else";
        case TokenType::KeywordFor: return "for";
        case TokenType::KeywordWhile: return "while";
        case TokenType::KeywordDo: return "do";
        case TokenType::KeywordSwitch: return "switch";
        case TokenType::KeywordMatch: return "match";
        case TokenType::KeywordCase: return "case";
        case TokenType::KeywordDefault: return "default";
        case TokenType::KeywordBreak: return "break";
        case TokenType::KeywordContinue: return "continue";
        case TokenType::KeywordReturn: return "return";

        case TokenType::KeywordImport: return "import";
        case TokenType::KeywordModule: return "module";
        case TokenType::KeywordNew: return "new";
        case TokenType::KeywordThis: return "this";
        case TokenType::KeywordSuper: return "super";
        case TokenType::KeywordNull: return "null";

        case TokenType::KeywordVar: return "var";
        case TokenType::KeywordLet: return "let";
        case TokenType::KeywordConst: return "const";

        case TokenType::KeywordPublic: return "public";
        case TokenType::KeywordPrivate: return "private";
        case TokenType::KeywordProtected: return "protected";
        case TokenType::KeywordInternal: return "internal";
        case TokenType::KeywordStatic: return "static";
        case TokenType::KeywordAbstract: return "abstract";
        case TokenType::KeywordOverride: return "override";
        case TokenType::KeywordExtends: return "extends";
        case TokenType::KeywordImplements: return "implements";
        case TokenType::KeywordWith: return "with";
        case TokenType::KeywordWeak: return "weak";

        case TokenType::KeywordAsync: return "async";
        case TokenType::KeywordAwait: return "await";
        case TokenType::KeywordThread: return "thread";
        case TokenType::KeywordSpawn: return "spawn";
        case TokenType::KeywordChannel: return "channel";
        case TokenType::KeywordMutex: return "mutex";
        case TokenType::KeywordTry: return "try";
        case TokenType::KeywordCatch: return "catch";
        case TokenType::KeywordFinally: return "finally";
        case TokenType::KeywordThrow: return "throw";
        case TokenType::KeywordUnsafe: return "unsafe";
        case TokenType::KeywordExtern: return "extern";

        case TokenType::KeywordTest: return "test";
        case TokenType::KeywordAssert: return "assert";

        case TokenType::Comment: return "Comment";
        case TokenType::DocComment: return "DocComment";
        case TokenType::At: return "@";

        case TokenType::KeywordVoid: return "void";
        case TokenType::KeywordInt: return "int";
        case TokenType::KeywordUint: return "uint";
        case TokenType::KeywordInt8: return "int8";
        case TokenType::KeywordInt16: return "int16";
        case TokenType::KeywordInt32: return "int32";
        case TokenType::KeywordInt64: return "int64";
        case TokenType::KeywordUint8: return "uint8";
        case TokenType::KeywordUint16: return "uint16";
        case TokenType::KeywordUint32: return "uint32";
        case TokenType::KeywordUint64: return "uint64";
        case TokenType::KeywordFloat: return "float";
        case TokenType::KeywordDouble: return "double";
        case TokenType::KeywordBool: return "bool";
        case TokenType::KeywordChar: return "char";
        case TokenType::KeywordString: return "string";
        case TokenType::KeywordByte: return "byte";

        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Percent: return "%";

        case TokenType::EqualEqual: return "==";
        case TokenType::BangEqual: return "!=";
        case TokenType::Less: return "<";
        case TokenType::Greater: return ">";
        case TokenType::LessEqual: return "<=";
        case TokenType::GreaterEqual: return ">=";

        case TokenType::AmpAmp: return "&&";
        case TokenType::PipePipe: return "||";
        case TokenType::Bang: return "!";

        case TokenType::Amp: return "&";
        case TokenType::Pipe: return "|";
        case TokenType::Caret: return "^";
        case TokenType::Tilde: return "~";
        case TokenType::LessLess: return "<<";
        case TokenType::GreaterGreater: return ">>";

        case TokenType::Equal: return "=";
        case TokenType::PlusEqual: return "+=";
        case TokenType::MinusEqual: return "-=";
        case TokenType::StarEqual: return "*=";
        case TokenType::SlashEqual: return "/=";
        case TokenType::PercentEqual: return "%=";

        case TokenType::PlusPlus: return "++";
        case TokenType::MinusMinus: return "--";

        case TokenType::Arrow: return "->";
        case TokenType::FatArrow: return "=>";
        case TokenType::QuestionDot: return "?.";
        case TokenType::QuestionQuestion: return "??";
        case TokenType::DotDot: return "..";

        case TokenType::OpenParen: return "(";
        case TokenType::CloseParen: return ")";
        case TokenType::OpenBrace: return "{";
        case TokenType::CloseBrace: return "}";
        case TokenType::OpenBracket: return "[";
        case TokenType::CloseBracket: return "]";
        case TokenType::Semicolon: return ";";
        case TokenType::Colon: return ":";
        case TokenType::Comma: return ",";
        case TokenType::Dot: return ".";
        case TokenType::Question: return "?";
    }
    return "<Unknown>";
}

} // namespace sp::lexer
