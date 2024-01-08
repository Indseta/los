#include <program/lexer.h>


const char *Lexer::keywords[] = {"if", "else", "bool", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "f32", "f64", "string"};
const char *Lexer::operators[] = {"=", "+", "-", "*", "/"};

const char Lexer::punctuators[] = {';', '.', ',', '{', '}', '[', ']', '(', ')'};


const std::vector<Lexer::Token> Lexer::lex(const std::string &source) {
    std::vector<Token> tokens;
    Token current_token;

    for (int i = 0; i < source.length(); ++i) {
        const char c = source[i];

        // Skip whitespaces
        if (std::isspace(c)) {
            continue;
        }

        // Reset token value
        current_token.value = "";

        // Check if character is a digit (Integer Literal)
        if (std::isdigit(c)) {
            current_token.value += c;
            while (i + 1 < source.length() && std::isdigit(source[i + 1])) {
                current_token.value += source[++i];
            }
            current_token.category = INTEGER_LITERAL;
        }

        // Check if character is a word (Keyword or Identifier)
        else if (std::isalpha(c)) {
            current_token.value += c;
            while (i + 1 < source.length() && std::isalnum(source[i + 1])) {
                current_token.value += source[++i];
            }
            if (is_keyword(current_token.value)) {
                current_token.category = KEYWORD;
            } else {
                current_token.category = IDENTIFIER;
            }
        }

        // Check for Operators
        else {
            bool found = false;
            current_token.value += c;
            for (const auto& op : operators) {
                if (current_token.value == op) {
                    current_token.category = OPERATOR;
                    found = true;
                    break;
                }
            }

            // Check for Punctuators
            if (!found) {
                for (const auto& punc : punctuators) {
                    if (c == punc) {
                        current_token.category = PUNCTUATOR;
                        found = true;
                        break;
                    }
                }
            }

            // Unknown character
            if (!found) {
                current_token.category = UNKNOWN;
            }
        }

        tokens.push_back(current_token);
    }

    return tokens;
}


const std::string Lexer::category_to_string(const Lexer::Category &category) {
    switch (category) {
        case PUNCTUATOR: return "punctuator";
        case KEYWORD: return "keyword";
        case IDENTIFIER: return "identifier";
        case OPERATOR: return "operator";
        case INTEGER_LITERAL: return "integer_literal";
        case STRING_LITERAL: return "string_literal";
        case LINE_COMMENT: return "line_comment";
        case BLOCK_COMMENT: return "block_comment";
        default: return "unknown";
    }
}


const bool Lexer::is_keyword(const std::string &value) {
    for (const auto &keyword : keywords) {
        if (value == keyword) {
            return true;
        }
    }

    return false;
}


std::ostream& operator<<(std::ostream &os, const Lexer::Token &token) {
    os << "(" << Lexer::category_to_string(token.category) << "): '" << token.value << "'";

    return os;
}