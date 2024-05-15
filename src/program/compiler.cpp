#include <program/compiler.h>

Compiler::Compiler(const IRGenerator &ir_generator, const std::string &fp) : fp(fp) {
    success = true;

    file_stream.open(fp + ".asm");

    if (!file_stream.is_open()) {
        success = false;
        throw std::runtime_error("Failed to open asm file");
    }

    compile(ir_generator);

    file_stream.close();

    if (run_cmd("nasm.exe -f win64 -g -o " + fp + ".o " + fp + ".asm")) {
        std::cout << "Assembly failed." << '\n';
        success = false;
        return;
    }

    if (run_cmd("gcc.exe -m64 -g " + fp + ".o -o " + fp)) {
        std::cout << "Linking failed." << '\n';
        success = false;
        return;
    }

    run_cmd("del " + fp + ".o");
    run_cmd("del " + fp + ".asm");
}

Compiler::~Compiler() {
    if (file_stream.is_open()) {
        file_stream.close();
    }
}

void Compiler::run() {
    run_cmd("call " + fp + ".exe");
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
            file_stream << '\t' << db->id << " db " << db->value << ", " << db->terminator << '\n';
        }
    }
    file_stream << '\n';
    file_stream << "segment .bss" << '\n';
    for (const auto &declaration : ir_generator.get_bss().declarations) {
        if (const auto *resb = dynamic_cast<const IRGenerator::Resb*>(declaration.get())) {
            file_stream << '\t' << resb->id << " resb " << resb->fac << '\n';
        }
        if (const auto *resw = dynamic_cast<const IRGenerator::Resw*>(declaration.get())) {
            file_stream << '\t' << resw->id << " resw " << resw->fac << '\n';
        }
        if (const auto *resd = dynamic_cast<const IRGenerator::Resd*>(declaration.get())) {
            file_stream << '\t' << resd->id << " resd " << resd->fac << '\n';
        }
        if (const auto *resq = dynamic_cast<const IRGenerator::Resq*>(declaration.get())) {
            file_stream << '\t' << resq->id << " resq " << resq->fac << '\n';
        }
    }
    file_stream << '\n';
    file_stream << "segment .text" << '\n';
    for (const auto &d : ir_generator.get_text().declarations) {
        if (const auto *entry = dynamic_cast<const IRGenerator::Entry*>(d.get())) {
            file_stream << '\t' << "global " << entry->id << '\n';
        }
    }
    file_stream << '\n';
    for (const auto &d : ir_generator.get_text().declarations) {
        if (const auto *entry = dynamic_cast<const IRGenerator::Entry*>(d.get())) {
            file_stream << entry->id << ':' << '\n';
            for (const auto &instruction : entry->instructions) {
                if (const auto *push_instr = dynamic_cast<const IRGenerator::Push*>(instruction.get())) {
                    file_stream << '\t' << "push " << push_instr->src << '\n';
                } else if (const auto *mov_instr = dynamic_cast<const IRGenerator::Mov*>(instruction.get())) {
                    file_stream << '\t' << "mov " << mov_instr->dst << ", " << mov_instr->src << '\n';
                } else if (const auto *movsx_instr = dynamic_cast<const IRGenerator::Movsx*>(instruction.get())) {
                    file_stream << '\t' << "movsx " << movsx_instr->dst << ", " << movsx_instr->src << '\n';
                } else if (const auto *lea_instr = dynamic_cast<const IRGenerator::Lea*>(instruction.get())) {
                    file_stream << '\t' << "lea " << lea_instr->dst << ", " << lea_instr->src << '\n';
                } else if (const auto *neg_instr = dynamic_cast<const IRGenerator::Neg*>(instruction.get())) {
                    file_stream << '\t' << "neg " << neg_instr->dst << '\n';
                } else if (const auto *imul_instr = dynamic_cast<const IRGenerator::Imul*>(instruction.get())) {
                    file_stream << '\t' << "imul " << imul_instr->dst << ", " << imul_instr->src << '\n';
                } else if (const auto *idiv_instr = dynamic_cast<const IRGenerator::Idiv*>(instruction.get())) {
                    file_stream << '\t' << "idiv " << idiv_instr->src << '\n';
                } else if (const auto *add_instr = dynamic_cast<const IRGenerator::Add*>(instruction.get())) {
                    file_stream << '\t' << "add " << add_instr->dst << ", " << add_instr->src << '\n';
                } else if (const auto *sub_instr = dynamic_cast<const IRGenerator::Sub*>(instruction.get())) {
                    file_stream << '\t' << "sub " << sub_instr->dst << ", " << sub_instr->src << '\n';
                } else if (const auto *Cmp_instr = dynamic_cast<const IRGenerator::Cmp*>(instruction.get())) {
                    file_stream << '\t' << "cmp " << Cmp_instr->left << ", " << Cmp_instr->right << '\n';
                } else if (const auto *Cmove_instr = dynamic_cast<const IRGenerator::Cmove*>(instruction.get())) {
                    file_stream << '\t' << "cmove " << Cmove_instr->dst << ", " << Cmove_instr->src << '\n';
                } else if (const auto *xor_instr = dynamic_cast<const IRGenerator::Xor*>(instruction.get())) {
                    file_stream << '\t' << "xor " << xor_instr->dst << ", " << xor_instr->src << '\n';
                } else if (const auto *leave_instr = dynamic_cast<const IRGenerator::Leave*>(instruction.get())) {
                    file_stream << '\t' << "leave" << '\n';
                } else if (const auto *ret_instr = dynamic_cast<const IRGenerator::Ret*>(instruction.get())) {
                    file_stream << '\t' << "ret" << '\n';
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