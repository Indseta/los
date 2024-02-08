#pragma once

#include <program/lexer.h>

#include <iostream>
#include <memory>
#include <set>
#include <vector>

class Parser {
public:
    struct Node {
        virtual ~Node() = default;
        virtual void log() const = 0;
    };

    struct VariableDeclaration : public Node {
        VariableDeclaration(const std::string &op, const std::string &identifier, std::unique_ptr<Node> expr) : op(std::move(op)), identifier(std::move(identifier)), expr(std::move(expr)) {}
        void log() const override {}
        std::string op;
        std::string identifier;
        std::unique_ptr<Node> expr;
    };

    struct BinaryOperation : public Node {
        BinaryOperation(std::unique_ptr<Node> left, const std::string &op, std::unique_ptr<Node> right) : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
        void log() const override {}
        std::unique_ptr<Node> left;
        std::string op;
        std::unique_ptr<Node> right;
    };

    struct UnaryOperation : public Node {
        UnaryOperation(const std::string &op, std::unique_ptr<Node> value) : op(std::move(op)), value(std::move(value)) {}
        void log() const override {}
        std::string op;
        std::unique_ptr<Node> value;
    };

    struct VariableCall : public Node {
        VariableCall(const std::string &identifier) : identifier(std::move(identifier)) {}
        void log() const override {}
        std::string identifier;
    };

    struct FunctionCall : public Node {
        FunctionCall(const std::string &identifier) : identifier(std::move(identifier)) {}
        void log() const override {}
        std::string identifier;
        std::vector<std::unique_ptr<Node>> args;
    };

    struct IntegerLiteral : public Node {
        IntegerLiteral(const int &value) : value(std::move(value)) {}
        void log() const override {}
        int value;
    };

    struct FloatLiteral : public Node {
        FloatLiteral(const float &value) : value(std::move(value)) {}
        void log() const override {}
        float value;
    };

    struct BooleanLiteral : public Node {
        BooleanLiteral(const bool &value) : value(std::move(value)) {}
        void log() const override {}
        bool value;
    };

    struct StringLiteral : public Node {
        StringLiteral(const std::string &value) : value(std::move(value)) {}
        void log() const override {}
        std::string value;
    };

    Parser(const Lexer &lexer);
    const std::vector<std::unique_ptr<Node>>& get() const;

private:
    std::vector<std::unique_ptr<Node>> ast;
    const std::vector<Lexer::Token> &tokens;
    size_t current = 0;

    void parse();
    bool isAtEnd() const;
    const Lexer::Token& peek() const;
    const Lexer::Token& previous() const;
    bool match(std::initializer_list<std::string> types);
    const Lexer::Token& advance();
    const Lexer::Token& consume(const std::string& type, const std::string& message);
    [[noreturn]] void error(const std::string &msg) const;

    std::unique_ptr<Node> statement();
    std::unique_ptr<Node> log_statement();
    std::unique_ptr<Node> variable_declaration();
    std::unique_ptr<Node> function_call();
    std::unique_ptr<Node> expression();
    std::unique_ptr<Node> equality();
    std::unique_ptr<Node> comparison();
    std::unique_ptr<Node> term();
    std::unique_ptr<Node> factor();
    std::unique_ptr<Node> unary();
    std::unique_ptr<Node> primary();
    const bool stob(const std::string &value) const;
};