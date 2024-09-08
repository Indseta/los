#include <program/compiler.h>

Compiler::Compiler(const IRGenerator &ir_generator, const std::string &src) {
    success = true;

    file_stream.open(src + ".asm");

    if (!file_stream.is_open()) {
        success = false;
        throw std::runtime_error("Failed to open asm file");
    }

    compile(ir_generator);

    file_stream.close();

    if (Utils::run_cmd("nasm.exe -f win64 -g -o " + src + ".o " + src + ".asm")) {
        std::cout << "Assembly failed." << '\n';
        success = false;
        return;
    }

    Utils::run_cmd("del \"" + src + ".asm\"");
}

Compiler::~Compiler() {
    if (file_stream.is_open()) {
        file_stream.close();
    }
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
    file_stream << "exit:" << '\n';
    file_stream << '\t' << "leave" << '\n';
    file_stream << '\t' << "ret" << '\n';
    for (const auto &d : ir_generator.get_text().declarations) {
        if (const auto *entry = dynamic_cast<const IRGenerator::Entry*>(d.get())) {
            file_stream << entry->id << ':' << '\n';
            for (const auto &instruction : entry->instructions) {
                compile_instruction(instruction.get());
            }
        }
    }
    for (const auto &d : ir_generator.get_labels()) {
        if (const auto *entry = dynamic_cast<const IRGenerator::Entry*>(d.get())) {
            file_stream << entry->id << ':' << '\n';
            for (const auto &instruction : entry->instructions) {
                compile_instruction(instruction.get());
            }
        }
    }
}

void Compiler::compile_instruction(const IRGenerator::Instruction *instruction) {
    if (const auto *push_instr = dynamic_cast<const IRGenerator::Push*>(instruction)) {
        file_stream << '\t' << "push " << push_instr->src << '\n';
    } else if (const auto *mov_instr = dynamic_cast<const IRGenerator::Mov*>(instruction)) {
        file_stream << '\t' << "mov " << mov_instr->dst << ", " << mov_instr->src << '\n';
    } else if (const auto *movsx_instr = dynamic_cast<const IRGenerator::Movsx*>(instruction)) {
        file_stream << '\t' << "movsx " << movsx_instr->dst << ", " << movsx_instr->src << '\n';
    } else if (const auto *lea_instr = dynamic_cast<const IRGenerator::Lea*>(instruction)) {
        file_stream << '\t' << "lea " << lea_instr->dst << ", " << lea_instr->src << '\n';
    } else if (const auto *neg_instr = dynamic_cast<const IRGenerator::Neg*>(instruction)) {
        file_stream << '\t' << "neg " << neg_instr->dst << '\n';
    } else if (const auto *imul_instr = dynamic_cast<const IRGenerator::Imul*>(instruction)) {
        file_stream << '\t' << "imul " << imul_instr->dst << ", " << imul_instr->src << '\n';
    } else if (const auto *idiv_instr = dynamic_cast<const IRGenerator::Idiv*>(instruction)) {
        file_stream << '\t' << "idiv " << idiv_instr->src << '\n';
    } else if (const auto *add_instr = dynamic_cast<const IRGenerator::Add*>(instruction)) {
        file_stream << '\t' << "add " << add_instr->dst << ", " << add_instr->src << '\n';
    } else if (const auto *sub_instr = dynamic_cast<const IRGenerator::Sub*>(instruction)) {
        file_stream << '\t' << "sub " << sub_instr->dst << ", " << sub_instr->src << '\n';
    } else if (const auto *cmp_instr = dynamic_cast<const IRGenerator::Cmp*>(instruction)) {
        file_stream << '\t' << "cmp " << cmp_instr->left << ", " << cmp_instr->right << '\n';
    } else if (const auto *sete_instr = dynamic_cast<const IRGenerator::Sete*>(instruction)) {
        file_stream << '\t' << "sete " << sete_instr->dst << '\n';
    } else if (const auto *setne_instr = dynamic_cast<const IRGenerator::Setne*>(instruction)) {
        file_stream << '\t' << "setne " << setne_instr->dst << '\n';
    } else if (const auto *setg_instr = dynamic_cast<const IRGenerator::Setg*>(instruction)) {
        file_stream << '\t' << "setg " << setg_instr->dst << '\n';
    } else if (const auto *setge_instr = dynamic_cast<const IRGenerator::Setge*>(instruction)) {
        file_stream << '\t' << "setge " << setge_instr->dst << '\n';
    } else if (const auto *setl_instr = dynamic_cast<const IRGenerator::Setl*>(instruction)) {
        file_stream << '\t' << "setl " << setl_instr->dst << '\n';
    } else if (const auto *setle_instr = dynamic_cast<const IRGenerator::Setle*>(instruction)) {
        file_stream << '\t' << "setle " << setle_instr->dst << '\n';
    } else if (const auto *cmove_instr = dynamic_cast<const IRGenerator::Cmove*>(instruction)) {
        file_stream << '\t' << "cmove " << cmove_instr->dst << ", " << cmove_instr->src << '\n';
    } else if (const auto *xor_instr = dynamic_cast<const IRGenerator::Xor*>(instruction)) {
        file_stream << '\t' << "xor " << xor_instr->dst << ", " << xor_instr->src << '\n';
    } else if (const auto *label_instr = dynamic_cast<const IRGenerator::Label*>(instruction)) {
        file_stream << label_instr->id << ":" << '\n';
    } else if (const auto *jmp_instr = dynamic_cast<const IRGenerator::Jmp*>(instruction)) {
        file_stream << '\t' << "jmp " << jmp_instr->dst << '\n';
    } else if (const auto *je_instr = dynamic_cast<const IRGenerator::Je*>(instruction)) {
        file_stream << '\t' << "je " << je_instr->dst << '\n';
    } else if (const auto *jne_instr = dynamic_cast<const IRGenerator::Jne*>(instruction)) {
        file_stream << '\t' << "jne " << jne_instr->dst << '\n';
    } else if (const auto *leave_instr = dynamic_cast<const IRGenerator::Leave*>(instruction)) {
        file_stream << '\t' << "leave" << '\n';
    } else if (const auto *ret_instr = dynamic_cast<const IRGenerator::Ret*>(instruction)) {
        file_stream << '\t' << "ret" << '\n';
    } else if (const auto *call_instr = dynamic_cast<const IRGenerator::Call*>(instruction)) {
        file_stream << '\t' << "call " << call_instr->id << '\n';
    }
}

const bool& Compiler::get_success() const {
    return success;
}