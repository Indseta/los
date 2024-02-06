#pragma once

#include <program/parser.h>

#include <program/parser.h>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

class Interpreter {
public:
    Interpreter(const Parser &parser);
    void interpret(const std::vector<std::unique_ptr<Parser::Node>> &ast);
    void log() const;

private:
    int evaluate(const Parser::Node* node);

    std::unordered_map<std::string, int> memory;
};