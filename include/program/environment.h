#pragma once

#define NDEBUG

#include <iostream>

#include <program/source.h>
#include <program/lexer.h>
#include <program/parser.h>
#include <program/interpreter.h>
#include <program/compiler.h>

class Environment {
public:
    void run(const std::string &fp);
};