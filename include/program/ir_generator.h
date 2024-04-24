#pragma once

#include <iostream>
#include <random>
#include <vector>

#include <program/parser.h>

class IRGenerator {
public:
    IRGenerator(const Parser &parser);

private:
    void generate_ir(const std::vector<std::unique_ptr<Parser::Node>> &ast);

    void evaluate_statement(const Parser::Node *expr);
    void evaluate_expr(const Parser::Node *expr);
    void evaluate_function_call(const Parser::FunctionCall *expr);
    void evaluate_unary_operation(const Parser::UnaryOperation *expr);
    void evaluate_binary_operation(const Parser::BinaryOperation *expr);

    std::string get_hash(const size_t &length);
};