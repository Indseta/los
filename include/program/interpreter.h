#pragma once

#include <program/parser.h>

#include <program/parser.h>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

class Interpreter {
public:
    struct Res {
        Res() : value(""), type("") {}
        Res(const std::string &value, const std::string &type) : value(value), type(type) {}
        std::string value;
        std::string type;
    };

    Interpreter(const Parser &parser);
    void interpret(const std::vector<std::unique_ptr<Parser::Node>> &ast);
    void log() const;

private:
    Res evaluate(const Parser::Node* node);

    const std::string add(const std::string &a, const std::string &b) const;
    const std::string sub(const std::string &a, const std::string &b) const;
    const std::string mul(const std::string &a, const std::string &b) const;
    const std::string div(const std::string &a, const std::string &b) const;

    std::unordered_map<std::string, Res> memory;
};