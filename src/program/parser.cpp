#include <program/parser.h>

Parser::Parser(const Lexer &lexer) {
    parse(lexer.get());

    for (const auto &node : ast) {
        node->print();
        std::cout << std::endl;
    }
}

void Parser::parse(const std::vector<Lexer::Token> &lex) {
    std::vector<Lexer::Token> stack;

    for (const auto &token : lex) {
        if (token.value == ";") {
            auto expression_tree = evaluate_stack(stack);
            if (expression_tree) {
                ast.push_back(std::move(expression_tree));
            }
            stack.clear();
        } else {
            stack.push_back(token);
        }
    }
}

std::unique_ptr<Parser::ASTNode> Parser::evaluate_stack(std::vector<Lexer::Token> &stack) {
    if (stack.empty()) return nullptr;

    if (stack[0].category == Lexer::KEYWORD && stack[0].value == Lexer::keywords.at("var_assign") && stack[1].category == Lexer::IDENTIFIER && stack[2].value == Lexer::operators.at("assign")) {
        auto var_assign = std::make_unique<VariableAssignment>();
        var_assign->type = stack[0].value;
        var_assign->identifier = stack[1].value;

        std::vector<Lexer::Token> expressionTokens(stack.begin() + 3, stack.end());
        var_assign->expression = parse_stack(expressionTokens);

        return var_assign;
    } else if (stack[0].category == Lexer::IDENTIFIER && stack[0].value == "println") {
        auto log_expr = std::make_unique<PrintExpression>();
		std::vector<Lexer::Token> expressionTokens(stack.begin() + 2, stack.end() - 1);
		log_expr->expression = parse_stack(expressionTokens);

		return log_expr;
	}

    return nullptr;
}

std::unique_ptr<Parser::ASTNode> Parser::parse_stack(std::vector<Lexer::Token> &stack) {
    std::stack<std::unique_ptr<ASTNode>> nodes;
    std::stack<std::string> ops;

    auto get_precedence = [](const std::string& op) -> int {
        if (op == "*" || op == "/") return 2;
        if (op == "+" || op == "-") return 1;
        return 0;
    };

    for (auto& token : stack) {
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