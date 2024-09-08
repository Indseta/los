#pragma once

#define NDEBUG

#include <chrono>
#include <iostream>

#include <program/source.h>
#include <program/lexer.h>
#include <program/parser.h>
#include <program/ir_generator.h>
#include <program/compiler.h>
#include <program/utils.h>

class Object {
public:
    Object(const std::string &src_id, const std::string &out_dir);
    void build();
    void log();

private:
    void compile();

    const std::string &src_id;
    const std::string &out_dir;

    bool success;
};