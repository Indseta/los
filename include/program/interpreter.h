#pragma once

#include <program/parser.h>

#include <program/parser.h>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

class Interpreter {
public:
	Interpreter(const Parser &parser);
	void run(const std::vector<std::unique_ptr<Parser::ASTNode>> &ast);
    void interpret(const std::vector<std::unique_ptr<Parser::ASTNode>> &ast);
    void print() const;

private:
    int evaluate(const Parser::ASTNode* node);

    std::unordered_map<std::string, int> memory;
};