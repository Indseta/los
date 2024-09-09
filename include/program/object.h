#pragma once

#define NDEBUG

#include <chrono>
#include <iostream>

#include <config/source.fwd.h>
#include <config/lexer.fwd.h>
#include <config/parser.fwd.h>
#include <config/compiler.fwd.h>
#include <config/ir_generator.fwd.h>

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