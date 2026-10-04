#pragma once

namespace sp::parser {

enum class Precedence {
    None,
    Assignment,     // = += -= *= /= %=
    Ternary,        // ? :
    NullCoalescing, // ??
    LogicalOr,      // ||
    LogicalAnd,     // &&
    BitwiseOr,      // |
    BitwiseXor,     // ^
    BitwiseAnd,     // &
    Equality,       // == !=
    Relational,     // < > <= >=
    Shift,          // << >>
    Range,          // ..
    Additive,       // + -
    Multiplicative, // * / %
    Unary,          // ! - + ~ ++ --
    Call,           // () [] . ?.
    Primary
};

} // namespace sp::parser
