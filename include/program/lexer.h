#pragma once


#include <cctype>
#include <iostream>
#include <string>
#include <vector>


class Lexer {
    private:
        static const char *keywords[];
        static const char *operators[];

        static const char punctuators[];

        enum Category {
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
            Category category = Category::UNKNOWN;
            std::string value = "";
        };

    public:
        static const std::vector<Token> lex(const std::string &source);
        static const std::string category_to_string(const Category &category);

    private:
        Lexer();

        static const bool is_keyword(const std::string &value);

        friend std::ostream& operator<<(std::ostream &os, const Token &token);
};