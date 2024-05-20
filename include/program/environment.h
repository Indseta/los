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
private:
    enum System {
        SYSTEM_WIN64,
        SYSTEM_WIN32,
        SYSTEM_LINUX,
        SYSTEM_APPLE,
        SYSTEM_UNKNOWN,
    };

public:
    void run(const std::string &src, const std::string &out, const bool &should_run = true);

#if _WIN32
#if defined(_WIN64)
    static const System system = SYSTEM_WIN64;
#else
    static const System system = SYSTEM_WIN32;
#endif
#elif __linux__
    static const System system = SYSTEM_LINUX;
#elif __APPLE__
static const System system = SYSTEM_APPLE;
#else
    static const System system = SYSTEM_UNKNOWN;
#endif
};