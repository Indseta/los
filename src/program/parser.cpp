#include <program/parser.h>


Parser::Parser(const std::string source_path) : lexer(source_path) {
    parse();

    // Debug: Print the AST
    for (const auto &node : ast) {
        node->print();
        std::cout << std::endl;
    }
}


void Parser::parse() {
    std::vector<Lexer::Token> stack;

    for (const auto &token : lexer.get_tokens()) {
        if (token.value == ";") {
            auto expression_tree = parse_expression(stack);
            if (expression_tree) {
                ast.push_back(std::move(expression_tree));
            }
            stack.clear();
        } else {
            stack.push_back(token);
        }
    }
}


std::unique_ptr<Parser::ExpressionTree> Parser::parse_expression(std::vector<Lexer::Token> &stack) {
    auto expression_tree = std::make_unique<ExpressionTree>();

    // Assuming the simple case of variable assignment with a binary expression
    if (stack.size() >= 5 && 
        stack[0].category == Lexer::TokenCategory::KEYWORD && 
        stack[1].category == Lexer::TokenCategory::IDENTIFIER && 
        stack[2].category == Lexer::TokenCategory::OPERATOR && 
        stack[2].value == "=") {
        
        // Create a variable assignment node
        auto var_assign = std::make_unique<VariableAssignment>();
        var_assign->type = stack[0].value;
        var_assign->identifier = stack[1].value;

        // Create a binary expression node
        auto bin_exp = std::make_unique<BinaryExpression>();
        bin_exp->left = std::make_unique<NumberLiteral>();
        static_cast<NumberLiteral*>(bin_exp->left.get())->value = stack[3].value;
        bin_exp->op = stack[4].value;
        bin_exp->right = std::make_unique<NumberLiteral>();
        static_cast<NumberLiteral*>(bin_exp->right.get())->value = stack[5].value;

        var_assign->expression = std::move(bin_exp);
        expression_tree->expression = std::move(var_assign);
    }

    return expression_tree;
}


// (keyword): 'i32'
// (identifier): 'x'
// (operator): '='
// (integer_literal): '25'
// (operator): '+'
// (integer_literal): '5'

// (expression tree): ''