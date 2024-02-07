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

    struct LogStatement : public Node {
        std::unique_ptr<Node> expr;
        explicit LogStatement(std::unique_ptr<Node> expr) : expr(std::move(expr)) {}
        void log() const override {}
    };

    struct VariableDeclaration : public Node {
        VariableDeclaration(std::string op, std::string identifier, std::unique_ptr<Node> expr) : op(std::move(op)), identifier(std::move(identifier)), expr(std::move(expr)) {}
        std::string op;
        std::string identifier;
        std::unique_ptr<Node> expr;
        void log() const override {}
    };

    struct BinaryOperation : public Node {
        std::unique_ptr<Node> left;
        std::string op;
        std::unique_ptr<Node> right;
        BinaryOperation(std::unique_ptr<Node> left, std::string op, std::unique_ptr<Node> right) : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
        void log() const override {}
    };

    struct UnaryOperation : public Node {
        std::string op;
        std::unique_ptr<Node> right;

        UnaryOperation(std::string op, std::unique_ptr<Node> right) : op(std::move(op)), right(std::move(right)) {}

        void log() const override {}
    };

    struct NumberLiteral : public Node {
        std::string value;
        explicit NumberLiteral(const std::string& value) : value(std::move(value)) {}
        void log() const override {}
    };

    struct StringLiteral : public Node {
        std::string value;
        explicit StringLiteral(const std::string& value) : value(std::move(value)) {}
        void log() const override {}
    };

    struct VariableCall : public Node {
        std::string value;
        explicit VariableCall(const std::string& value) : value(std::move(value)) {}
        void log() const override {}
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
    std::unique_ptr<Node> expression();
    std::unique_ptr<Node> equality();
    std::unique_ptr<Node> comparison();
    std::unique_ptr<Node> term();
    std::unique_ptr<Node> factor();
    std::unique_ptr<Node> unary();
    std::unique_ptr<Node> primary();
};