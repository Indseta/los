// bits 64
// default rel

// extern ExitProcess
// extern printf

// segment .data
// 	   msgc5f1a964 db "Hello world", 0xd, 0xa, 0
// 	   msge0255f52 db "%d", 0xd, 0xa, 0

// segment .text
// 	   global main

// main:
// 	   push rbp
// 	   mov rbp, rsp
// 	   sub rsp, 32

// 	   lea rcx, [msgc5f1a964]
// 	   call printf
// 	   lea rcx, [msge0255f52]
// 	   mov edx, 2763
// 	   call printf

// 	   xor rax, rax
// 	   call ExitProcess

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
    if (const auto *node = dynamic_cast<const Parser::Entry*>(expr)) {
    } else if (const auto *node = dynamic_cast<const Parser::FunctionCall*>(expr)) {
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

std::string IRGenerator::get_hash(const size_t &length) {
    const char legal_chars[] = "0123456789abcdef";

    std::random_device rd;
    std::mt19937 generator(rd());

    std::uniform_int_distribution<> distribution(0, sizeof(legal_chars) - 2);

    std::string res;

    for (size_t i = 0; i < length; ++i) {
        res += legal_chars[distribution(generator)];
    }

    return res;
}
