#include <program/lexer.h>

const std::unordered_map<std::string, std::string> Lexer::keywords = {
	{"cnd_entry", "if"},
	{"cnd_option", "elif"},
	{"cnd_exit", "else"},
	{"var_assign", "let"},
};

const std::unordered_map<std::string, std::string> Lexer::operators = {
	{"assign", "="},
	{"plus", "+"},
	{"minus", "-"},
	{"multiply", "*"},
	{"divide", "/"},
};

const std::unordered_map<std::string, std::string> Lexer::punctuators = {
	{"end", ";"},
	{"obj_sep", "."},
	{"elm_sep", ","},
	{"group_open", "("},
	{"group_close", ")"},
	{"memsp_open", "{"},
	{"memsp_close", "}"},
	{"subsc_open", "["},
	{"subsc_close", "]"},
};

Lexer::Lexer(const Source &source) {
    lex(source.get());

	for (const auto &t : tokens) {
		std::cout << t << '\n';
	}
}

void Lexer::lex(const std::string &source) {
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
                if (current_token.value == op.second) {
                    current_token.category = OPERATOR;
                    found = true;
                    break;
                }
            }

            // Check for punctuators
            if (!found) {
                for (const auto& punc : punctuators) {
                    if (punc.second[0] == c) {
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
}

const std::string Lexer::to_string(const Lexer::TokenCategory &category) {
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
        if (value == keyword.second) {
            return true;
        }
    }

    return false;
}

const std::vector<Lexer::Token>& Lexer::get() const {
    return tokens;
}

std::ostream& operator<<(std::ostream &os, const Lexer::Token &token) {
    os << "(" << Lexer::to_string(token.category) << "): '" << token.value << "'";

    return os;
}