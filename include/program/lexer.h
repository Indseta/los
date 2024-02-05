#pragma once

#include <program/source.h>

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class Lexer {
private:
    static const char *keywords[];
    static const char *operators[];

    static const char punctuators[];

public:
    enum TokenCategory {
        PUNCTUATOR,
        KEYWORD,
        IDENTIFIER,
        OPERATOR,
        INTEGER_LITERAL,
        STRING_LITERAL,
        LINE_COMMENT,
        BLOCK_COMMENT,
        UNKNOWN,
    };

    struct Token {
        TokenCategory category = TokenCategory::UNKNOWN;
        std::string value = "";
    };

    Lexer(const Source &source);

    const std::vector<Token>& get() const;

private:
    const std::vector<Token> lex(const std::string &source) const;
    const bool is_keyword(const std::string &value) const;
    static const std::string to_string(const TokenCategory &category);

    std::vector<Token> tokens;

    friend std::ostream& operator<<(std::ostream &os, const Token &token);
};