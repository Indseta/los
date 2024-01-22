#include <program/parser.h>


Parser::Parser(const std::string source_path) : lexer(source_path) {
    parse();
}


void Parser::parse() {
    std::vector<Lexer::Token> stack;

    for (const auto &token : lexer.get_tokens()) {
        if (token.value == ";") {
            parse_expression(stack);
            stack.clear();
        } else {
            stack.push_back(token);
        }
    }
}


Parser::ExpressionTree Parser::parse_expression(std::vector<Lexer::Token> &stack) {
    ExpressionTree expression_tree;

    return expression_tree;
}


// (keyword): 'i32'
// (identifier): 'x'
// (operator): '='
// (integer_literal): '25'
// (operator): '+'
// (integer_literal): '5'

// (expression tree): ''