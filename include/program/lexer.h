#pragma once


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

public:
    struct Token {
        TokenCategory category = TokenCategory::UNKNOWN;
        std::string value = "";
    };

    Lexer(const std::string source_path);

    const std::vector<Token>& get_tokens() const;

private:
    const std::string get_source(const std::string &source_path) const;
    const std::vector<Token> lex(const std::string &source) const;

    const bool is_keyword(const std::string &value) const;

    static const std::string category_to_string(const TokenCategory &category);

    std::vector<Token> tokens;

    friend std::ostream& operator<<(std::ostream &os, const Token &token);
};