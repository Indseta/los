#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <variant>
#include <stdexcept>
#include <program/parser.h>

class Interpreter {
public:
    using Value = std::variant<int, float, bool, std::string>;

    struct Variable {
        Value value;

        void log() const {
            std::visit([](auto&& arg) {
                std::cout << arg << '\n';
            }, value);
        }
    };

    struct Function {
        Function(std::vector<std::string> params, std::unique_ptr<Parser::Node> statement) : params(std::move(params)), statement(std::move(statement)) {}
        std::vector<std::string> params;
        std::unique_ptr<Parser::Node> statement;
    };

    Interpreter(const Parser &parser);

    void interpret(const std::vector<std::unique_ptr<Parser::Node>> &ast);
    void interpret_node(const Parser::Node *expr);

private:
    std::unordered_map<std::string, Value> heap;
    std::unordered_map<std::string, Function> functions;

    Value evaluate_node(const Parser::Node *node);
    Value evaluate_unary_operation(const Parser::UnaryOperation *expr);
    Value evaluate_binary_operation(const Parser::BinaryOperation *expr);

    bool custom_compare(const Value &left, const Value &right);

    Value perform_comparison_operation(const Value &left, const Value &right, const std::string &op);
    Value perform_arithmetic_operation(const Value &left, const Value &right, const std::string &op);

    float modulo(const float &a, const float &b);
};