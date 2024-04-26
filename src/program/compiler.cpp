#include <program/compiler.h>

Compiler::Compiler(const IRGenerator &ir_generator, const std::string &fp) {
    success = true;

    sfn = fp;
    std::string ext = ".los";

    size_t pos = sfn.find(ext);
    if (pos != std::string::npos) {
        sfn.erase(pos, ext.length());
    }

    file_stream.open(sfn + ".asm");

    if (!file_stream.is_open()) {
        success = false;
        throw std::runtime_error("Failed to open asm file");
    }

    compile(ir_generator);

    file_stream.close();

    if (run_cmd("nasm.exe -f win64 -g -o " + sfn + ".o " + sfn + ".asm")) {
        std::cout << "Assembly failed." << '\n';
        success = false;
        return;
    }

    if (run_cmd("gcc.exe -m64 -g " + sfn + ".o -o " + sfn)) {
        std::cout << "Linking failed." << '\n';
        success = false;
        return;
    }

    run_cmd("del " + sfn + ".o");
    run_cmd("del " + sfn + ".asm");
}

Compiler::~Compiler() {
    if (file_stream.is_open()) {
        file_stream.close();
    }
}

void Compiler::run() {
    run_cmd("call " + sfn + ".exe");
}

void Compiler::compile(const IRGenerator &ir_generator) {
    file_stream << "bits 64" << '\n';
    file_stream << "default rel" << '\n';
    file_stream << '\n';
    for (const auto &l : ir_generator.get_ext_libs()) {
        file_stream << "extern " << l << '\n';
    }
    file_stream << '\n';
    file_stream << "segment .data" << '\n';
    for (const auto &declaration : ir_generator.get_data().declarations) {
        if (const auto *db = dynamic_cast<const IRGenerator::Db*>(declaration.get())) {
            file_stream << '\t' << db->id << " db " << db->value << ", 0xd, 0xa, 0" << '\n';
        }
    }
    file_stream << '\n';
    file_stream << "segment .text" << '\n';
    for (const auto &d : ir_generator.get_text().declarations) {
        if (const auto *entry = dynamic_cast<const IRGenerator::Entry*>(d.get())) {
            file_stream << '\t' << "global " << entry->id << '\n';
        }
    }

    for (const auto &d : ir_generator.get_text().declarations) {
        if (const auto *entry = dynamic_cast<const IRGenerator::Entry*>(d.get())) {
            file_stream << entry->id << ':' << '\n';
            for (const auto &instruction : entry->instructions) {
                if (const auto *push_instr = dynamic_cast<const IRGenerator::Push*>(instruction.get())) {
                    file_stream << '\t' << "push " << push_instr->src << '\n';
                } else if (const auto *mov_instr = dynamic_cast<const IRGenerator::Mov*>(instruction.get())) {
                    file_stream << '\t' << "mov " << mov_instr->dst << ", " << mov_instr->src << '\n';
                } else if (const auto *lea_instr = dynamic_cast<const IRGenerator::Lea*>(instruction.get())) {
                    file_stream << '\t' << "lea " << lea_instr->dst << ", " << lea_instr->src << '\n';
                } else if (const auto *imul_instr = dynamic_cast<const IRGenerator::Imul*>(instruction.get())) {
                    file_stream << '\t' << "imul " << imul_instr->dst << ", " << imul_instr->src << '\n';
                } else if (const auto *idiv_instr = dynamic_cast<const IRGenerator::Idiv*>(instruction.get())) {
                    file_stream << '\t' << "idiv " << idiv_instr->src << '\n';
                } else if (const auto *add_instr = dynamic_cast<const IRGenerator::Add*>(instruction.get())) {
                    file_stream << '\t' << "add " << add_instr->dst << ", " << add_instr->src << '\n';
                } else if (const auto *sub_instr = dynamic_cast<const IRGenerator::Sub*>(instruction.get())) {
                    file_stream << '\t' << "sub " << sub_instr->dst << ", " << sub_instr->src << '\n';
                } else if (const auto *xor_instr = dynamic_cast<const IRGenerator::Xor*>(instruction.get())) {
                    file_stream << '\t' << "xor " << xor_instr->dst << ", " << xor_instr->src << '\n';
                } else if (const auto *call_instr = dynamic_cast<const IRGenerator::Call*>(instruction.get())) {
                    file_stream << '\t' << "call " << call_instr->id << '\n';
                }
            }
        }
    }
}

int Compiler::run_cmd(const std::string &cmd) {
    return system(cmd.c_str());
}

const bool& Compiler::get_success() const {
    return success;
}