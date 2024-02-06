#include <program/parser.h>

Parser::Parser(const Lexer &lexer) {
    parse(lexer.get());
}

void Parser::parse(const std::vector<Lexer::Token> &lex) {
    std::vector<Lexer::Token> stack;

    for (const auto &t : lex) {
        if (t.value == ";") {
            auto expr = expr_selector(stack);
            if (expr) {
                ast.push_back(std::move(expr));
            }
            stack.clear();
        } else {
            stack.push_back(t);
        }
    }
}

std::unique_ptr<Parser::Node> Parser::expr_selector(std::vector<Lexer::Token> &expr) {
    if (expr.empty()) {
        return nullptr;
    }

    if (match_keyword(expr, 0, "var_assign")) {
        auto node = std::make_unique<VariableAssignment>();
        node->type = expr[0].value;
        node->identifier = expr[1].value;
        std::vector<Lexer::Token> node_stack(expr.begin() + 3, expr.end());
        node->expr = parse_expr(node_stack);
        return node;
    } else if (match_identifier(expr, 0, "println")) {
        auto node = std::make_unique<LogStatement>();
        std::vector<Lexer::Token> node_stack(expr.begin() + 2, expr.end() - 1);
        node->expr = parse_expr(node_stack);
        return node;
    } else {
        return nullptr;
    }
}

std::unique_ptr<Parser::Node> Parser::parse_expr(std::vector<Lexer::Token> &expr) {
    std::stack<std::unique_ptr<Node>> nodes;
    std::stack<std::string> ops;

    auto get_precedence = [](const std::string& op) -> int {
        if (op == "*" || op == "/") return 2;
        if (op == "+" || op == "-") return 1;
        if (op == "(") return 3; 
        return 0;
    };

    for (auto &t : expr) {
        if (t.category == Lexer::INTEGER_LITERAL) {
            nodes.push(std::make_unique<NumberLiteral>(t.value));
        } else if (t.category == Lexer::IDENTIFIER) {
            nodes.push(std::make_unique<VariableCall>(t.value));
        } else if (t.category == Lexer::PUNCTUATOR && t.value == "(") {
            ops.push(t.value);
        } else if (t.category == Lexer::PUNCTUATOR && t.value == ")") {
            while (!ops.empty() && ops.top() != "(") {
                auto right = std::move(nodes.top());
                nodes.pop();
                auto left = std::move(nodes.top());
                nodes.pop();

                auto op = ops.top();
                ops.pop();

                auto expr = std::make_unique<BinaryOperation>();
                expr->left = std::move(left);
                expr->op = op;
                expr->right = std::move(right);
                nodes.push(std::move(expr));
            }
            if (!ops.empty()) ops.pop();
        } else if (t.category == Lexer::OPERATOR) {
            while (!ops.empty() && get_precedence(ops.top()) >= get_precedence(t.value) && ops.top() != "(") {
                auto right = std::move(nodes.top());
                nodes.pop();
                auto left = std::move(nodes.top());
                nodes.pop();

                auto op = ops.top();
                ops.pop();

                auto expr = std::make_unique<BinaryOperation>();
                expr->left = std::move(left);
                expr->op = op;
                expr->right = std::move(right);
                nodes.push(std::move(expr));
            }
            ops.push(t.value);
        }
    }

    while (!ops.empty()) {
        auto right = std::move(nodes.top());
        nodes.pop();
        auto left = std::move(nodes.top());
        nodes.pop();

        auto op = ops.top();
        ops.pop();

        auto expr = std::make_unique<BinaryOperation>();
        expr->left = std::move(left);
        expr->op = op;
        expr->right = std::move(right);
        nodes.push(std::move(expr));
    }

    return std::move(nodes.top());
}

bool Parser::match_keyword(const std::vector<Lexer::Token> &expr, const size_t &i, const std::string &key) {
    return (expr[i].category == Lexer::KEYWORD && expr[i].value == Lexer::keywords.at(key));
}

bool Parser::match_identifier(const std::vector<Lexer::Token> &expr, const size_t &i, const std::string &value) {
    return (expr[i].category == Lexer::IDENTIFIER && expr[i].value == value);
}

const std::vector<std::unique_ptr<Parser::Node>>& Parser::get() const {
    return ast;
}