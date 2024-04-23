#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <program/parser.h>

class Compiler {
public:
    Compiler(const Parser &parser, const std::string &fp);
    ~Compiler();

private:
    void compile(const std::vector<std::unique_ptr<Parser::Node>> &ast);

    void evaluate_statement(const Parser::Node *expr);
    void evaluate_expr(const Parser::Node *expr, size_t &ix);
    void evaluate_function_call(const Parser::FunctionCall *expr, size_t &ix);
    void evaluate_unary_operation(const Parser::UnaryOperation *expr, size_t &ix);
    void evaluate_binary_operation(const Parser::BinaryOperation *expr, size_t &ix);

    int run_cmd(const std::string &cmd);
    std::string get_hash(const size_t &length);

    std::ofstream file_stream;

    std::vector<std::string> segment_data;
    std::vector<std::string> segment_text;
    std::vector<std::string> extern_fns;
    std::vector<std::string> entry_main;
};