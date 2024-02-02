#include <program/lexer.h>


const char *Lexer::keywords[] = {"if", "else", "bool", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "f32", "f64", "string"};
const char *Lexer::operators[] = {"=", "+", "-", "*", "/"};

const char Lexer::punctuators[] = {';', '.', ',', '{', '}', '[', ']', '(', ')'};


Lexer::Lexer(const std::string source_path) {
    const std::string source = get_source(source_path);
    tokens = lex(source);
	for (const auto &t : tokens) {
		std::cout << t << '\n';
	}
}


const std::string Lexer::get_source(const std::string &source_path) const {
    std::ifstream file(source_path); // "../src/" + source_path

    if (!file.is_open()) {
        std::cerr << "Failed to open file" << '\n';
        return "";
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    file.close();

    return ss.str();
}


const std::vector<Lexer::Token> Lexer::lex(const std::string &source) const {
    std::vector<Lexer::Token> tokens;
    Token current_token;

    for (int i = 0; i < source.length(); ++i) {
        const char c = source[i];

        // Skip whitespaces
        if (std::isspace(c)) {
            continue;
        }

        // Reset token value
        current_token.value = "";

        // Check if character is a digit
        if (std::isdigit(c)) {
            current_token.value += c;
            while (i + 1 < source.length() && std::isdigit(source[i + 1])) {
                current_token.value += source[++i];
            }
            current_token.category = INTEGER_LITERAL;
        }

        // Check for keyword or identifier
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

        // Check for operators
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

            // Check for punctuators
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


const std::string Lexer::category_to_string(const Lexer::TokenCategory &category) {
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


const bool Lexer::is_keyword(const std::string &value) const {
    for (const auto &keyword : keywords) {
        if (value == keyword) {
            return true;
        }
    }

    return false;
}


const std::vector<Lexer::Token>& Lexer::get_tokens() const {
    return tokens;
}


std::ostream& operator<<(std::ostream &os, const Lexer::Token &token) {
    os << "(" << Lexer::category_to_string(token.category) << "): '" << token.value << "'";

    return os;
}