#include <program/interpreter.h>

Interpreter::Interpreter(const Parser &parser) {
    interpret(parser.get());
}

void Interpreter::interpret(const std::vector<std::unique_ptr<Parser::Node>> &ast) {
    for (const auto &tree : ast) {
        if (const auto *expr = dynamic_cast<const Parser::Node*>(tree.get())) {
            interpret_node(expr);
        } else {
            std::cout << "Unknown node type encountered" << '\n';
        }
    }
}

void Interpreter::interpret_node(const Parser::Node *expr) {
    if (const auto *node = dynamic_cast<const Parser::VariableDeclaration*>(expr)) {
        auto res = evaluate_node(node->expr.get());
        heap[node->identifier] = std::move(res);
    } else if (const auto *node = dynamic_cast<const Parser::ScopeDeclaration*>(expr)) {
        interpret(node->ast);
    } else if (const auto *node = dynamic_cast<const Parser::ConditionalStatement*>(expr)) {
        auto res = evaluate_node(node->condition.get());
        if (auto res_bool = dynamic_cast<Bool*>(res.get())) {
            if (res_bool->value) {
                interpret_node(node->pass_statement.get());
            } else {
                interpret_node(node->fail_statement.get());
            }
        } else {
            throw std::runtime_error("Expected a boolean expression");
        }
    } else if (const auto *node = dynamic_cast<const Parser::PassStatement*>(expr)) {
    } else if (const auto *node = dynamic_cast<const Parser::WhileLoopStatement*>(expr)) {
        if (auto res = dynamic_cast<Bool*>(evaluate_node(node->condition.get()).get())) {
            while (res->value) {
                interpret_node(node->statement.get());
                res = dynamic_cast<Bool*>(evaluate_node(node->condition.get()).get());
            }
        } else {
            throw std::runtime_error("Expected a boolean expression");
        }
    } else if (const auto *node = dynamic_cast<const Parser::FunctionCall*>(expr)) {
        std::vector<std::unique_ptr<Type>> args;
        for (const auto &a : node->args) {
            auto res = evaluate_node(a.get());
            args.push_back(std::move(res));
        }
        if (node->identifier == "println") {
            switch (args.size()) {
            case 0:
                throw std::runtime_error("Too few arguments in function call: " + node->identifier);
                break;
            default:
                for (const auto &a : args) {
                    a->log();
                }
                break;
            }
        } else {}
    } else {
        std::cout << "Unsupported statement encountered:" << '\n';
        expr->log();
        std::cout << '\n';
    }
}

std::unique_ptr<Interpreter::Type> Interpreter::evaluate_node(const Parser::Node *node) {
    if (const auto *expr = dynamic_cast<const Parser::IntegerLiteral*>(node)) {
        return std::make_unique<Int>(expr->value);
    } else if (const auto* expr = dynamic_cast<const Parser::FloatLiteral*>(node)) {
        return std::make_unique<Float>(expr->value);
    } else if (const auto* expr = dynamic_cast<const Parser::BooleanLiteral*>(node)) {
        return std::make_unique<Bool>(expr->value);
    } else if (const auto* expr = dynamic_cast<const Parser::StringLiteral*>(node)) {
        return std::make_unique<String>(expr->value);
    } else if (const auto *expr = dynamic_cast<const Parser::VariableCall*>(node)) {
        auto r = heap.find(expr->identifier);
        if (r != heap.end()) {
            return r->second->clone();
        } else {
            throw std::runtime_error("Undefined variable: " + expr->identifier);
        }
    } else if (const auto *expr = dynamic_cast<const Parser::BinaryOperation*>(node)) {
        auto left_ptr = evaluate_node(expr->left.get());
        auto right_ptr = evaluate_node(expr->right.get());

        if (auto left = dynamic_cast<Int*>(left_ptr.get()), right = dynamic_cast<Int*>(right_ptr.get()); left && right) {
            if (expr->op == "==") {
                return std::make_unique<Bool>(left->value == right->value);
            } else if (expr->op == "!=") {
                return std::make_unique<Bool>(left->value != right->value);
            } else if (expr->op == "<") {
                return std::make_unique<Bool>(left->value < right->value);
            } else if (expr->op == ">") {
                return std::make_unique<Bool>(left->value > right->value);
            } else if (expr->op == "<=") {
                return std::make_unique<Bool>(left->value <= right->value);
            } else if (expr->op == ">=") {
                return std::make_unique<Bool>(left->value >= right->value);
            } else if (expr->op == "+") {
                return std::make_unique<Int>(left->value + right->value);
            } else if (expr->op == "-") {
                return std::make_unique<Int>(left->value - right->value);
            } else if (expr->op == "*") {
                return std::make_unique<Int>(left->value * right->value);
            } else if (expr->op == "/") {
                if (right->value == 0) throw std::runtime_error("Division by zero");
                return std::make_unique<Int>(left->value / right->value);
            } else if (expr->op == "%") {
                return std::make_unique<Int>(left->value % right->value);
            } else {
                throw std::runtime_error("Unsupported operator: " + expr->op);
            }
        } else {
            throw std::runtime_error("Type mismatch or unsupported types for binary operation");
        }
    } else {
        throw std::runtime_error("Unsupported expression encountered");
    }
}