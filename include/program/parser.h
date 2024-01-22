#pragma once


#include <program/lexer.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>


class Parser {
public:
    struct ASTNode {
        virtual ~ASTNode() = default;
    };

    struct ExpressionTree : ASTNode {
        ASTNode expression;
    };

    struct VariableAssignment : ASTNode {
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
    
    struct VariableCall : ASTNode {
        std::string value;
    };

    Parser(const std::string source_path);

private:
    void parse();
    ExpressionTree parse_expression(std::vector<Lexer::Token> &stack);

    const Lexer lexer;

    std::vector<ASTNode> ast;
};