#pragma once

#include <program/source.h>

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class Lexer {
public:
    static const std::vector<std::string> keywords;
    static const std::vector<std::string> operators;
    static const std::vector<std::string> punctuators;

    enum TokenCategory {
        PUNCTUATOR,
        KEYWORD,
        IDENTIFIER,
        OPERATOR,
        INTEGER_LITERAL,
        FLOAT_LITERAL,
        BOOLEAN_LITERAL,
        STRING_LITERAL,
        LINE_COMMENT,
        BLOCK_COMMENT,
        UNKNOWN,
    };

    struct Token {
        TokenCategory category = TokenCategory::UNKNOWN;
        size_t line;
        std::string value = "";
        void log() const;
    };

    Lexer(const Source &source);

    const std::vector<Token>& get() const;

    const bool& get_success() const;

private:
    void lex(const std::string &raw);
    const bool is_keyword(const std::string &value) const;
    const bool is_operator(const std::string &value) const;
    const bool is_punctuator(const std::string &value) const;

    std::vector<Token> tokens;

    bool success;
};