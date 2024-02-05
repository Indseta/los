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
    struct ASTNode {
        virtual void print() const = 0;
        virtual ~ASTNode() = default;
    };

    struct Stack : ASTNode {
        std::unique_ptr<ASTNode> expression;
        void print() const override {
            if (expression) expression->print();
        }
    };

    struct VariableAssignment : ASTNode {
        std::string type;
        std::string identifier;
        std::unique_ptr<ASTNode> expression;
        void print() const override {
            std::cout << "Variable Assignment: Type = " << type << ", Identifier = " << identifier << ", Expression = ";
            if (expression) expression->print();
            else std::cout << "null";
            std::cout << std::endl;
        }
    };

    struct BinaryExpression : ASTNode {
        std::unique_ptr<ASTNode> left;
        std::string op;
        std::unique_ptr<ASTNode> right;
        void print() const override {
            std::cout << "(";
            if (left) left->print();
            std::cout << " " << op << " ";
            if (right) right->print();
            std::cout << ")";
        }
    };

    struct NumberLiteral : ASTNode {
		explicit NumberLiteral(const std::string& val) : value(val) {}
        std::string value;
        void print() const override {
            std::cout << "NumberLiteral(" << value << ")";
        }
    };
    
    struct VariableCall : ASTNode {
		explicit VariableCall(const std::string& val) : value(val) {}
        std::string value;
        void print() const override {
            std::cout << "VariableCall(" << value << ")";
        }
    };

    Parser(const Lexer &lexer);

	const std::vector<std::unique_ptr<ASTNode>>& get() const;

private:
    void parse(const std::vector<Lexer::Token> &lex);

    std::unique_ptr<ASTNode> parse_expression(std::vector<Lexer::Token> &stack);
	std::unique_ptr<ASTNode> parse_math_expression(std::vector<Lexer::Token> &tokens);

    std::vector<std::unique_ptr<ASTNode>> ast;
};