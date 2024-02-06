#include <program/lexer.h>

const std::unordered_map<std::string, std::string> Lexer::keywords = {
    {"var_type_string", "string"},
    {"var_type_int8", "i8"},
    {"var_type_int16", "i16"},
    {"var_type_int32", "i32"},
    {"var_type_int64", "i64"},
    {"var_type_float8", "f8"},
    {"var_type_float16", "f16"},
    {"var_type_float32", "f32"},
    {"var_type_float64", "f64"},
    {"var_type_bool", "bool"},
    {"var_assign", "var"},
    {"func_assign", "function"},
    {"func_exit", "return"},
    {"cnd_entry", "if"},
    {"cnd_exit", "else"},
    {"loop_cls", "for"},
    {"loop_cnd", "while"},
    {"loop_exit", "break"},
    {"loop_skip", "continue"},
};

const std::unordered_map<std::string, std::string> Lexer::operators = {
    {"assign", "="},
    {"plus", "+"},
    {"minus", "-"},
    {"multiply", "*"},
    {"divide", "/"},
    {"equality", "=="},
    {"inequality", "!="},
    {"greater", ">"},
    {"greater_equal", ">="},
    {"less", ">"},
    {"less_equal", ">="},
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
}

void Lexer::lex(const std::string &raw) {
    Token ct;

    for (int i = 0; i < raw.length(); ++i) {
        const char c = raw[i];

        // Skip whitespace
        if (std::isspace(c)) {
            continue;
        }

        // Reset token value
        ct.value = "";

        // Keyword & identifier
        if (std::isalpha(c)) {
            ct.value += c;
            while (i + 1 < raw.length() && (std::isalpha(raw[i + 1]) || std::isdigit(raw[i + 1]))) {
                ct.value += raw[++i];
            }
            ct.category = is_keyword(ct.value) ? KEYWORD : IDENTIFIER;
            tokens.push_back(ct);
            continue;
        }

        // Integer literal
        if (std::isdigit(c)) {
            ct.value += c;
            while (i + 1 < raw.length() && std::isdigit(raw[i + 1])) {
                ct.value += raw[++i];
            }
            ct.category = INTEGER_LITERAL;
            tokens.push_back(ct);
            continue;
        }

        // Operator
        if (is_operator(std::string(1, c))) {
            ct.value += c;
            while (i + 1 < raw.length() && is_operator(ct.value + raw[i + 1])) {
                ct.value += raw[++i];
            }
            ct.category = OPERATOR;
            tokens.push_back(ct);
            continue;
        }

        // Punctuator
        if (is_punctuator(std::string(1, c))) {
            ct.value += c;
            while (i + 1 < raw.length() && is_punctuator(ct.value + raw[i + 1])) {
                ct.value += raw[++i];
            }
            ct.category = PUNCTUATOR;
            tokens.push_back(ct);
            continue;
        }

        // Unkown token
        ct.category = UNKNOWN;
        tokens.push_back(ct);
    }
}

const bool Lexer::is_keyword(const std::string &value) const {
    for (const auto &e : keywords) {
        if (value == e.second) {
            return true;
        }
    }
    return false;
}

const bool Lexer::is_operator(const std::string &value) const {
    for (const auto& e : operators) {
        if (value == e.second) {
            return true;
        }
    }
    return false;
}

const bool Lexer::is_punctuator(const std::string &value) const {
    for (const auto& e : punctuators) {
        if (value == e.second) {
            return true;
        }
    }
    return false;
}

const std::vector<Lexer::Token>& Lexer::get() const {
    return tokens;
}
