#pragma once


#include <program/lexer.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>


class Parser {
private:
    enum ASTNodeCategory {};

    struct ASTNode {
        virtual ~ASTNode() = default;
    };

    struct VariableDeclaration : ASTNode {
        std::string type;
        std::string identifier;
        std::unique_ptr<ASTNode> expression;
    };

    struct BinaryExpression : ASTNode {
        std::unique_ptr<ASTNode> left;
        std::string op;
        std::unique_ptr<ASTNode> right;
    };

    struct NumberLiteral : ASTNode {
        std::string value;
    };

    struct Identifier : ASTNode {
        std::string name;
    };

public:
    Parser(const std::string source_path);

private:
    std::unique_ptr<ASTNode> parse();

    void assign_expr();
    void add_expr();
    void mul_expr();
    void pow_expr();
    void unary_expr();
    void primary();

    const Lexer lexer;
};