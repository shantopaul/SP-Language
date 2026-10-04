#include "sp/lexer/Token.h"
#include <sstream>

namespace sp::lexer {

std::string Token::toString() const {
    std::ostringstream oss;
    oss << "Token(" << tokenTypeToString(type) << ", \"" << lexeme << "\", " << location.toString() << ")";
    return oss.str();
}

} // namespace sp::lexer
