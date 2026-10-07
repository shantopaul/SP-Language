#pragma once

#include <string_view>

namespace sp::lexer {

enum class TokenType {
    // Special
    EndOfFile,
    Invalid,

    // Literals
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    CharLiteral,
    BooleanLiteral,
    Identifier,

    // Keywords - Declarations
    KeywordFn,
    KeywordClass,
    KeywordInterface,
    KeywordStruct,
    KeywordEnum,
    KeywordTrait,

    // Keywords - Control Flow
    KeywordIf,
    KeywordElse,
    KeywordFor,
    KeywordWhile,
    KeywordDo,
    KeywordSwitch,
    KeywordMatch,
    KeywordCase,
    KeywordDefault,
    KeywordBreak,
    KeywordContinue,
    KeywordReturn,

    // Keywords - Modules & Objects
    KeywordImport,
    KeywordModule,
    KeywordNew,
    KeywordThis,
    KeywordSuper,
    KeywordNull,

    // Keywords - Variables
    KeywordVar,
    KeywordLet,
    KeywordConst,

    // Keywords - Modifiers
    KeywordPublic,
    KeywordPrivate,
    KeywordProtected,
    KeywordInternal,
    KeywordStatic,
    KeywordAbstract,
    KeywordOverride,
    KeywordExtends,
    KeywordImplements,
    KeywordWith,
    KeywordWeak,

    // Keywords - Concurrency & Systems
    KeywordAsync,
    KeywordAwait,
    KeywordThread,
    KeywordSpawn,
    KeywordChannel,
    KeywordMutex,
    KeywordTry,
    KeywordCatch,
    KeywordFinally,
    KeywordThrow,
    KeywordUnsafe,
    KeywordExtern,

    // Keywords - Testing
    KeywordTest,
    KeywordAssert,

    // Comments & Annotations
    Comment,
    DocComment,
    At,             // @
    Hash,           // #

    // Primitive Type Keywords
    KeywordVoid,
    KeywordInt,
    KeywordUint,
    KeywordInt8,
    KeywordInt16,
    KeywordInt32,
    KeywordInt64,
    KeywordUint8,
    KeywordUint16,
    KeywordUint32,
    KeywordUint64,
    KeywordFloat,
    KeywordDouble,
    KeywordBool,
    KeywordChar,
    KeywordString,
    KeywordByte,

    // Operators - Arithmetic
    Plus,
    Minus,
    Star,
    Slash,
    Percent,

    // Operators - Comparison
    EqualEqual,
    BangEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,

    // Operators - Logical
    AmpAmp,
    PipePipe,
    Bang,

    // Operators - Bitwise
    Amp,
    Pipe,
    Caret,
    Tilde,
    LessLess,
    GreaterGreater,

    // Operators - Assignment
    Equal,
    PlusEqual,
    MinusEqual,
    StarEqual,
    SlashEqual,
    PercentEqual,

    // Operators - Increment/Decrement
    PlusPlus,
    MinusMinus,

    // Operators - Navigation & Flow
    Arrow,          // ->
    FatArrow,       // =>
    QuestionDot,    // ?.
    QuestionQuestion,// ??
    DotDot,         // ..

    // Punctuation
    OpenParen,      // (
    CloseParen,     // )
    OpenBrace,      // {
    CloseBrace,     // }
    OpenBracket,    // [
    CloseBracket,   // ]
    Semicolon,      // ;
    Colon,          // :
    Comma,          // ,
    Dot,            // .
    Question        // ?
};

[[nodiscard]] std::string_view tokenTypeToString(TokenType type) noexcept;

} // namespace sp::lexer
