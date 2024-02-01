#pragma once

#include <program/parser.h>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

class Interpreter {
private:
    std::unordered_map<std::string, int> variables;

    int evaluate(const Parser::ASTNode* node);

public:
    void interpret(const std::vector<std::unique_ptr<Parser::ASTNode>>& ast);

    void debug_print() const;
};