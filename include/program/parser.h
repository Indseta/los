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
        virtual void log() const {}
    };

    struct BinaryOperation : public Node {
        BinaryOperation(std::unique_ptr<Node> left, const std::string &op, std::unique_ptr<Node> right) : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
        void log() const override {
            std::cout << "BinaryOperation: (left: (";
            left->log();
            std::cout << "), op: '" << op << "', right: (";
            right->log();
            std::cout << "))";
        }
        std::unique_ptr<Node> left;
        std::string op;
        std::unique_ptr<Node> right;
    };

    struct UnaryOperation : public Node {
        UnaryOperation(const std::string &op, std::unique_ptr<Node> value) : op(std::move(op)), value(std::move(value)) {}
        void log() const override {
            std::cout << "UnaryOperation: (op: '" << op << "', value: (";
            value->log();
            std::cout << "))";
        }
        std::string op;
        std::unique_ptr<Node> value;
    };

    struct IntegerLiteral : public Node {
        IntegerLiteral(const int &value) : value(std::move(value)) {}
        void log() const override {
            std::cout << "IntegerLiteral: '" << value << "'";
        }
        int value;
    };

    struct FloatLiteral : public Node {
        FloatLiteral(const float &value) : value(std::move(value)) {}
        void log() const override {
            std::cout << "FloatLiteral: '" << value << "'";
        }
        float value;
    };

    struct BooleanLiteral : public Node {
        BooleanLiteral(const bool &value) : value(std::move(value)) {}
        void log() const override {
            std::cout << "BooleanLiteral: '";
            if (value) {
                std::cout << "true";
            } else {
                std::cout << "false";
            }
            std::cout << "'";
        }
        bool value;
    };

    struct StringLiteral : public Node {
        StringLiteral(const std::string &value) : value(std::move(value)) {}
        void log() const override {
            std::cout << "StringLiteral: '" << value << "'";
        }
        std::string value;
    };

    struct VariableDeclaration : public Node {
        VariableDeclaration(const std::string &op, const std::string &identifier, std::unique_ptr<Node> expr) : op(std::move(op)), identifier(std::move(identifier)), expr(std::move(expr)) {}
        void log() const override {
            std::cout << "VariableDeclaration: (op: '" << op << "', identifier: '" << identifier << "', expr: (";
            expr->log();
            std::cout << "))";
        }
        std::string op;
        std::string identifier;
        std::unique_ptr<Node> expr;
    };

    struct VariableCall : public Node {
        VariableCall(const std::string &identifier) : identifier(std::move(identifier)) {}
        void log() const override {
            std::cout << "VariableCall: '" << identifier << "'";
        }
        std::string identifier;
    };

    struct FunctionCall : public Node {
        FunctionCall(const std::string &identifier) : identifier(std::move(identifier)) {}
        void log() const override {
            std::cout << "FunctionCall: (identifier: '" << identifier << "', args: (";
            for (size_t i = 0; i < args.size(); ++i) {
                const bool last = i == args.size() - 1;
                args[i]->log();
                if (!last) {
                    std::cout << ", ";
                }
            }
            std::cout << "))";
        }
        std::string identifier;
        std::vector<std::unique_ptr<Node>> args;
    };

    struct ConditionalStatement : public Node {
        ConditionalStatement() : pass_statement(std::make_unique<PassStatement>()), fail_statement(std::make_unique<PassStatement>()) {}
        void log() const override {
            std::cout << "ConditionalStatement: (condition: (";
            condition->log();
            std::cout << "), pass_statement: (";
            pass_statement->log();
            std::cout << "), fail_statement: (";
            fail_statement->log();
            std::cout << "))";
        }
        std::unique_ptr<Node> condition;
        std::unique_ptr<Node> pass_statement;
        std::unique_ptr<Node> fail_statement;
    };

    struct ScopeDeclaration : public Node {
        ScopeDeclaration() {}
        void log() const override {
            std::cout << "ScopeDeclaration: (ast: {" << '\n';
            for (const auto &n : ast) {
                std::cout << '\t';
                n->log();
                std::cout << '\n';
            }
            std::cout << "})";
        }
        std::vector<std::unique_ptr<Node>> ast;
    };

    struct PassStatement : public Node {
        PassStatement() {}
        void log() const override {
            std::cout << "PassStatement";
        }
    };

    struct WhileLoopStatement : public Node {
        WhileLoopStatement() {}
        void log() const override {
            std::cout << "WhileLoopStatement: (condition: (";
            condition->log();
            std::cout << "), statement: (";
            statement->log();
            std::cout << "))";
        }
        std::unique_ptr<Node> condition;
        std::unique_ptr<Node> statement;
    };

    Parser(const Lexer &lexer);
    const std::vector<std::unique_ptr<Node>>& get() const;

private:
    std::vector<std::unique_ptr<Node>> ast;
    const std::vector<Lexer::Token> &tokens;
    size_t current = 0;

    void parse();
    bool at_end() const;
    const Lexer::Token& peek() const;
    const Lexer::Token& next() const;
    const Lexer::Token& previous() const;
    bool match(std::initializer_list<std::string> types);
    const Lexer::Token& advance();
    const Lexer::Token& consume(const std::string& type, const std::string& message);
    [[noreturn]] void error(const std::string &msg) const;

    std::unique_ptr<Node> statement();
    std::unique_ptr<Node> conditional_statement();
    std::unique_ptr<Node> while_loop_statement();
    std::unique_ptr<Node> scope_declaration();
    std::unique_ptr<Node> variable_declaration();
    std::unique_ptr<Node> function_call();
    std::unique_ptr<Node> expression();
    std::unique_ptr<Node> equality();
    std::unique_ptr<Node> comparison();
    std::unique_ptr<Node> term();
    std::unique_ptr<Node> factor();
    std::unique_ptr<Node> remainder();
    std::unique_ptr<Node> unary();
    std::unique_ptr<Node> primary();
};