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


std::unique_ptr<Parser::ASTNode> Parser::parse_expression(std::vector<Lexer::Token> &tokens) {
    if (tokens.empty()) return nullptr;

    if (tokens[0].category == Lexer::KEYWORD && tokens[1].category == Lexer::IDENTIFIER && tokens[2].value == "=") {
		// Variable assignment
        auto var_assign = std::make_unique<VariableAssignment>();
        var_assign->type = tokens[0].value; // Assuming the first token is the type
        var_assign->identifier = tokens[1].value; // The variable name

        // Remove the first three tokens (type, variable, and =) before processing the expression
        std::vector<Lexer::Token> expressionTokens(tokens.begin() + 3, tokens.end());
        var_assign->expression = parse_math_expression(expressionTokens);

        return var_assign;
    }

    return nullptr;
}

std::unique_ptr<Parser::ASTNode> Parser::parse_math_expression(std::vector<Lexer::Token> &tokens) {
    std::unique_ptr<ASTNode> expression;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.category == Lexer::TokenCategory::INTEGER_LITERAL) {
            if (!expression) {
                // First number in the expression
                auto number = std::make_unique<NumberLiteral>();
                number->value = token.value;
                expression = std::move(number);
            } else if (i > 1 && tokens[i - 1].value == "+") {
                // Create a new binary expression for addition
                auto addition = std::make_unique<BinaryExpression>();
                addition->op = "+";

                auto rightNumber = std::make_unique<NumberLiteral>();
                rightNumber->value = token.value;

                addition->left = std::move(expression);
                addition->right = std::move(rightNumber);

                expression = std::move(addition);
            }
        } else if (token.category == Lexer::TokenCategory::IDENTIFIER) {
            if (!expression) {
                // First number in the expression
                auto number = std::make_unique<VariableCall>();
                number->value = token.value;
                expression = std::move(number);
            } else if (i > 1 && tokens[i - 1].value == "+") {
                // Create a new binary expression for addition
                auto addition = std::make_unique<BinaryExpression>();
                addition->op = "+";

                auto rightNumber = std::make_unique<VariableCall>();
                rightNumber->value = token.value;

                addition->left = std::move(expression);
                addition->right = std::move(rightNumber);

                expression = std::move(addition);
            }
		}
    }

    return expression;
}


// (keyword): 'i32'
// (identifier): 'x'
// (operator): '='
// (integer_literal): '25'
// (operator): '+'
// (integer_literal): '5'

// (expression tree): ''