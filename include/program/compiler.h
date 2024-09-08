#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#include <program/ir_generator.h>
#include <program/utils.h>

class Compiler {
public:
    Compiler(const IRGenerator &ir_generator, const std::string &src);
    ~Compiler();

    const bool& get_success() const;

private:
    void compile(const IRGenerator &ir_generator);

    void compile_instruction(const IRGenerator::Instruction *instruction);

    std::ofstream file_stream;

    bool success;
};