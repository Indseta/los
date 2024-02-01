#pragma once


#include <program/lexer.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>


class Parser {
public:
    struct ASTNode {
        virtual void print() const = 0;  // Added print function for debugging
        virtual ~ASTNode() = default;
    };

    struct ExpressionTree : ASTNode {
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
        std::string value;
        void print() const override {
            std::cout << "NumberLiteral(" << value << ")";
        }
    };
    
    struct VariableCall : ASTNode {
        std::string value;
        void print() const override {
            std::cout << "VariableCall(" << value << ")";
        }
    };

    Parser(const std::string source_path);

private:
    void parse();
    std::unique_ptr<Parser::ExpressionTree> parse_expression(std::vector<Lexer::Token> &stack);

    const Lexer lexer;

public:
    std::vector<std::unique_ptr<ASTNode>> ast;
};