/*
 * Copyright (c) 2012-2013 Dano Pernis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <string>
#include <map>
#include <iterator>
#include <istream>
#include <boost/optional.hpp>

namespace hcc {
namespace jack {

enum class TokenType {
// keyword
    CLASS,
    METHOD,
    FUNCTION,
    CONSTRUCTOR,
    INT,
    BOOLEAN,
    CHAR,
    VOID,
    VAR,
    STATIC,
    FIELD,
    LET,
    DO,
    IF,
    ELSE,
    WHILE,
    RETURN,
    TRUE,
    FALSE,
    NULL_,
    THIS,
// punctuation
    AMPERSAND,
    PARENTHESIS_LEFT,
    PARENTHESIS_RIGHT,
    ASTERISK,
    PLUS_SIGN,
    COMMA,
    MINUS_SIGN,
    DOT,
    SLASH,
    SEMICOLON,
    LESS_THAN_SIGN,
    EQUALS_SIGN,
    GREATER_THAN_SIGN,
    BRACKET_LEFT,
    BRACKET_RIGHT,
    BRACE_LEFT,
    VERTICAL_BAR,
    BRACE_RIGHT,
    TILDE,
// rest
    IDENTIFIER,
    INT_CONST,
    STRING_CONST,
    EOF_,
    STRAY_CHARACTER
};

std::ostream& operator<<(std::ostream& os, const TokenType& token);

class Tokenizer {
private:
    std::istreambuf_iterator<char> current;
    std::istreambuf_iterator<char> last;
    boost::optional<char> previousChar;

    TokenType type;
    std::string string;
    int intConstant;
    unsigned line, column;

    TokenType _advance();

public:
    Tokenizer(std::basic_istream<char>& is);

    void advance() {
        type = _advance();
    }

    TokenType getTokenType() const {
        return type;
    }

    const std::string getIdentifier() const {
        return string;
    }

    const std::string getStringConstant() const {
        return string;
    }

    int getIntConstant() const {
        return intConstant;
    }

    unsigned getLine() const {
        return line;
    }

    unsigned getColumn() const {
        return column;
    }
};

} // end namespace jack
} // end namespace hcc
