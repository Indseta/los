#pragma once

#include <program/lexer.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <vector>

class Parser {
public:
    struct Node {
        virtual void log() const = 0;
        virtual ~Node() = default;
    };

	struct ConsoleLog : Node {
        std::unique_ptr<Node> expr;
        void log() const override {
            std::cout << "Log: Expression = ";
            if (expr) {
                expr->log();
            } else {
                std::cout << "null";
            }
            std::cout << '\n';
        }
	};

    struct VariableAssignment : Node {
        std::string type;
        std::string identifier;
        std::unique_ptr<Node> expr;
        void log() const override {
            std::cout << "Variable Assignment: Type = " << type << ", Identifier = " << identifier << ", Expression = ";
            if (expr) {
                expr->log();
            } else {
                std::cout << "null";
            }
            std::cout << '\n';
        }
    };

    struct BinaryOperation : Node {
        std::unique_ptr<Node> left;
        std::string op;
        std::unique_ptr<Node> right;
        void log() const override {
            std::cout << "(";
            if (left) {
                left->log();
            }
            std::cout << " " << op << " ";
            if (right) {
                right->log();
            }
            std::cout << ")";
        }
    };

    struct NumberLiteral : Node {
		explicit NumberLiteral(const std::string& val) : value(val) {}
        std::string value;
        void log() const override {
            std::cout << "NumberLiteral(" << value << ")";
        }
    };
    
    struct VariableCall : Node {
		explicit VariableCall(const std::string& val) : value(val) {}
        std::string value;
        void log() const override {
            std::cout << "VariableCall(" << value << ")";
        }
    };

    Parser(const Lexer &lexer);

	const std::vector<std::unique_ptr<Node>>& get() const;

private:
    void parse(const std::vector<Lexer::Token> &lex);

    std::unique_ptr<Node> expr_selector(std::vector<Lexer::Token> &expr);
	std::unique_ptr<Node> parse_expr(std::vector<Lexer::Token> &expr);
	std::unique_ptr<Node> binop_expr(std::vector<Lexer::Token> &expr);

	bool match_keyword(const std::vector<Lexer::Token> &expr, const size_t &i, const std::string &key);
	bool match_identifier(const std::vector<Lexer::Token> &expr, const size_t &i, const std::string &value);

    std::vector<std::unique_ptr<Node>> ast;
};