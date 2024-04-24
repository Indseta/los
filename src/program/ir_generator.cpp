#include <program/ir_generator.h>

IRGenerator::IRGenerator(const Parser &parser) {
    generate_ir(parser.get());
}

void IRGenerator::generate_ir(const std::vector<std::unique_ptr<Parser::Node>> &ast) {
    for (const auto &t : ast) {
        evaluate_statement(t.get());
    }
}

void IRGenerator::evaluate_statement(const Parser::Node *expr) {
    if (const auto *node = dynamic_cast<const Parser::FunctionCall*>(expr)) {
        evaluate_function_call(node);
    } else if (const auto *node = dynamic_cast<const Parser::EmptyStatement*>(expr)) {
    } else {
        throw std::runtime_error("Unsupported statement encountered.");
    }
}

void IRGenerator::evaluate_expr(const Parser::Node *node) {
    if (const auto *child_node = dynamic_cast<const Parser::IntegerLiteral*>(node)) {
    } else if (const auto *child_node = dynamic_cast<const Parser::FloatLiteral*>(node)) {
    } else if (const auto *child_node = dynamic_cast<const Parser::BooleanLiteral*>(node)) {
    } else if (const auto *child_node = dynamic_cast<const Parser::StringLiteral*>(node)) {
    } else if (const auto *n = dynamic_cast<const Parser::UnaryOperation*>(node)) {
    } else if (const auto *n = dynamic_cast<const Parser::BinaryOperation*>(node)) {
        if (n->op == "*") {
        } else if (n->op == "/") {
        } else if (n->op == "+") {
        } else if (n->op == "-") {
        } else if (n->op == "%") {
        } else if (n->op == "==") {
        } else if (n->op == "!=") {
        } else if (n->op == ">") {
        } else if (n->op == ">=") {
        } else if (n->op == "<") {
        } else if (n->op == "<=") {
        } else {
            throw std::runtime_error("Unsupported operator.");
        }
    } else {
        throw std::runtime_error("Unsupported expression encountered.");
    }
}

void IRGenerator::evaluate_function_call(const Parser::FunctionCall *expr) {
    if (expr->identifier == "println") {
    } else {
        throw std::runtime_error("Function signature: \"" + expr->identifier + "\" with " + std::to_string(expr->args.size()) + " args not defined");
    }
}

void IRGenerator::evaluate_unary_operation(const Parser::UnaryOperation *expr) {
}

void IRGenerator::evaluate_binary_operation(const Parser::BinaryOperation *expr) {
}
