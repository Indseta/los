#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#include <program/ir_generator.h>

class Compiler {
public:
    Compiler(const IRGenerator &ir_generator, const std::string &fp);
    ~Compiler();

    const bool& get_success() const;
    void run();

private:
    void compile(const IRGenerator &ir_generator);

    int run_cmd(const std::string &cmd);

    std::ofstream file_stream;
    std::string sfn;

    bool success;
};