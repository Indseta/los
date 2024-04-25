#pragma once

#define NDEBUG

#include <chrono>
#include <iostream>

#include <program/source.h>
#include <program/lexer.h>
#include <program/parser.h>
#include <program/ir_generator.h>
#include <program/compiler.h>

class Environment {
public:
    void run(const std::string &fp);
};