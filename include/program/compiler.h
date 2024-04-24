#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <program/parser.h>

struct Registry{
    Registry();
    Registry(const std::string &qw, const std::string &dw, const std::string &w, const std::string &b);
    const std::string qw;
    const std::string dw;
    const std::string w;
    const std::string b;
};

struct Registries {
    Registries();

    static void next();
    static void back();

    static Registry rax;
    static Registry rbx;
    static Registry rcx;
    static Registry rdx;
    static Registry r8;
    static Registry r9;

    static Registry *areg;
};

class Compiler {
public:
    Compiler(const Parser &parser, const std::string &fp);
    ~Compiler();

private:
    void compile(const std::vector<std::unique_ptr<Parser::Node>> &ast);

    void evaluate_statement(const Parser::Node *expr);
    void evaluate_expr(const Parser::Node *expr);
    void evaluate_function_call(const Parser::FunctionCall *expr);
    void evaluate_unary_operation(const Parser::UnaryOperation *expr);
    void evaluate_binary_operation(const Parser::BinaryOperation *expr);

    int run_cmd(const std::string &cmd);
    std::string get_hash(const size_t &length);

    size_t ix;

    std::ofstream file_stream;

    std::vector<std::string> segment_data;
    std::vector<std::string> segment_text;
    std::vector<std::string> extern_fns;
    std::vector<std::string> entry_main;
};