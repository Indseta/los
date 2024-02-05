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
    static const std::unordered_map<std::string, std::string> keywords;
    static const std::unordered_map<std::string, std::string> operators;
    static const std::unordered_map<std::string, std::string> punctuators;

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
        void log() const {
            std::cout << "(";
            switch (category) {
                case PUNCTUATOR: std::cout << "punctuator";
                case KEYWORD: std::cout << "keyword";
                case IDENTIFIER: std::cout << "identifier";
                case OPERATOR: std::cout << "operator";
                case INTEGER_LITERAL: std::cout << "integer_literal";
                case STRING_LITERAL: std::cout << "string_literal";
                case LINE_COMMENT: std::cout << "line_comment";
                case BLOCK_COMMENT: std::cout << "block_comment";
                default: std::cout << "unknown";
            }
            std::cout << "): '" << value << "'" << '\n';
        }
    };

    Lexer(const Source &source);

    const std::vector<Token>& get() const;

private:
    void lex(const std::string &source);
    const bool is_keyword(const std::string &value) const;

    std::vector<Token> tokens;
};