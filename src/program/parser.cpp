#include <program/parser.h>

Parser::Parser(const Lexer &lexer) {
    parse(lexer.get());
}

void Parser::parse(const std::vector<Lexer::Token> &lex) {
    std::vector<Lexer::Token> stack;

    for (const auto &token : lex) {
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

    if (tokens[0].category == Lexer::KEYWORD && tokens[0].value == "let" && tokens[1].category == Lexer::IDENTIFIER && tokens[2].value == "=") {
        auto var_assign = std::make_unique<VariableAssignment>();
        var_assign->type = tokens[0].value;
        var_assign->identifier = tokens[1].value;

        std::vector<Lexer::Token> expressionTokens(tokens.begin() + 3, tokens.end());
        var_assign->expression = parse_math_expression(expressionTokens);

        return var_assign;
    }

    return nullptr;
}

std::unique_ptr<Parser::ASTNode> Parser::parse_math_expression(std::vector<Lexer::Token> &tokens) {
    std::stack<std::unique_ptr<ASTNode>> nodes;
    std::stack<std::string> ops;

    auto get_precedence = [](const std::string& op) -> int {
        if (op == "*" || op == "/") return 2;
        if (op == "+" || op == "-") return 1;
        return 0;
    };

    for (auto& token : tokens) {
        if (token.category == Lexer::INTEGER_LITERAL) {
            nodes.push(std::make_unique<NumberLiteral>(token.value));
        } else if (token.category == Lexer::IDENTIFIER) {
            nodes.push(std::make_unique<VariableCall>(token.value));
        } else if (token.category == Lexer::OPERATOR) {
            while (!ops.empty() && get_precedence(ops.top()) >= get_precedence(token.value)) {
                auto right = std::move(nodes.top());
                nodes.pop();
                auto left = std::move(nodes.top());
                nodes.pop();

                auto op = ops.top();
                ops.pop();

                auto expr = std::make_unique<BinaryExpression>();
                expr->left = std::move(left);
                expr->op = op;
                expr->right = std::move(right);
                nodes.push(std::move(expr));
            }
            ops.push(token.value);
        }
    }

    while (!ops.empty()) {
        auto right = std::move(nodes.top());
        nodes.pop();
        auto left = std::move(nodes.top());
        nodes.pop();

        auto op = ops.top();
        ops.pop();

        auto expr = std::make_unique<BinaryExpression>();
        expr->left = std::move(left);
        expr->op = op;
        expr->right = std::move(right);
        nodes.push(std::move(expr));
    }

    return std::move(nodes.top());
}

const std::vector<std::unique_ptr<Parser::ASTNode>>& Parser::get() const {
	return ast;
}