#include <program/terminal.h>

#include <iostream>
#include <chrono>
#include <fstream>
#include <regex>
#include <string>
#include <sstream>
#include <vector>


std::string get_source(std::string path) {
    std::ifstream file("../src/" + path);

    if (!file.is_open()) {
        std::cerr << "Failed to open file" << '\n';
        return "";
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    file.close();

    return ss.str();
}


enum TokenType {
    KEYWORD,
    IDENTIFIER,
    OPERATOR,
    PUNCTUATION,
    INTEGER_LITERAL,
    UNKNOWN,
};


struct Token {
    TokenType type;
    std::string value;
};


std::ostream& operator<<(std::ostream &os, const Token &token) {
    switch (token.type) {
        case KEYWORD:
            os << "(KEYWORD): " << token.value;
            break;
        case IDENTIFIER:
            os << "(IDENTIFIER): " << token.value;
            break;
        case OPERATOR:
            os << "(OPERATOR): " << token.value;
            break;
        case PUNCTUATION:
            os << "(PUNCTUATION): " << token.value;
            break;
        case INTEGER_LITERAL:
            os << "(INTEGER_LITERAL): " << token.value;
            break;
        case UNKNOWN:
        default:
            os << "(UNKNOWN): " << token.value;
            break;
    }

    return os;
}


std::vector<Token> lex(const std::string &source) {
    std::vector<Token> tokens;

    return tokens;
}


struct Node {};


int main() {
    cmd::clear();

    // Start program
    auto start = std::chrono::high_resolution_clock::now();

    const std::string src = get_source("main.pseudo");
    const auto tokens = lex(src);

    for (const auto &token : tokens) {
        std::cout << token << '\n';
    }

    // End program
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    cmd::ostream::newline();

    std::cout << "[Done] Program finished in " << elapsed.count() / 1000.0 << " seconds" << '\n';

    // Flush and pause console
    cmd::ostream::fbuffer();
    cmd::pause();

    return 0;
}