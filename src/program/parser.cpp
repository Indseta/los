#include <program/parser.h>


Parser::Parser(const std::string source_path) : lexer(source_path) {
    for (const auto &token : lexer.get_tokens()) {
        std::cout << token << '\n';
    }
}