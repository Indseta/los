#include <program/compiler.h>

Compiler::Compiler(const Parser &parser, const std::string &fp) {
    std::string sfn = fp;
    std::string ext = ".los";

    size_t pos = sfn.find(ext);
    if (pos != std::string::npos) {
        sfn.erase(pos, ext.length());
    }

    file_stream.open(sfn + ".asm");

    if (!file_stream.is_open()) {
        throw std::runtime_error("Failed to open asm file");
    }

    compile(parser.get());

    file_stream.close();

    if (run_cmd("nasm -f win64 -g -o " + sfn + ".o " + sfn + ".asm")) {
        std::cout << "Assembly failed." << '\n';
        return;
    }

    if (run_cmd("gcc -m64 -g " + sfn + ".o -o " + sfn + ".exe")) {
        std::cout << "Linking failed." << '\n';
        return;
    }

    // run_cmd("del " + sfn + ".o");
    // run_cmd("del " + sfn + ".asm");
    run_cmd("call " + sfn + ".exe");
}

Compiler::~Compiler() {
    if (file_stream.is_open()) {
        file_stream.close();
    }
}

void Compiler::compile(const std::vector<std::unique_ptr<Parser::Node>> &ast) {
    for (const auto &t : ast) {
        evaluate_statement(t.get());
    }

    file_stream << "bits 64" << '\n';
    file_stream << "default rel" << '\n';
    file_stream << '\n';
    file_stream << "extern ExitProcess" << '\n';
    for (const auto &v : extern_fns) {
        file_stream << "extern " << v << '\n';
    }
    file_stream << '\n';
    file_stream << "segment .data" << '\n';
    for (const auto &v : segment_data) {
        file_stream << '\t' << v << '\n';
    }
    file_stream << '\n';
    file_stream << "segment .text" << '\n';
    for (const auto &v : segment_text) {
        file_stream << '\t' << v << '\n';
    }
    file_stream << '\t' << "global main" << '\n';
    file_stream << '\n';
    file_stream << "main:" << '\n';
    file_stream << '\t' << "push rbp" << '\n';
    file_stream << '\t' << "mov rbp, rsp" << '\n';
    file_stream << '\t' << "sub rsp, 32" << '\n';
    file_stream << '\n';
    for (const auto &v : entry_main) {
        file_stream << '\t' << v << '\n';
    }
    file_stream << '\n';
    file_stream << '\t' << "xor rax, rax" << '\n';
    file_stream << '\t' << "call ExitProcess" << '\n';
}

void Compiler::evaluate_statement(const Parser::Node *expr) {
    size_t ix = entry_main.size();
    if (const auto *node = dynamic_cast<const Parser::FunctionCall*>(expr)) {
        evaluate_function_call(node, ix);
    } else if (const auto *node = dynamic_cast<const Parser::EmptyStatement*>(expr)) {
    } else {
        throw std::runtime_error("Unsupported node type encountered");
    }
}

void Compiler::evaluate_expr(const Parser::Node *node, size_t &ix) {
    if (const auto *child_node = dynamic_cast<const Parser::IntegerLiteral*>(node)) {
        std::string hash = "msg" + get_hash(8);
        segment_data.push_back(hash + " db \"%d\", 0xd, 0xa, 0");
        entry_main.insert(entry_main.begin() + ix, "lea rcx, [" + hash + "]");
        entry_main.insert(entry_main.begin() + ix, "mov edx, " + std::to_string(child_node->value));
    } else if (const auto *child_node = dynamic_cast<const Parser::FloatLiteral*>(node)) {
    } else if (const auto *child_node = dynamic_cast<const Parser::BooleanLiteral*>(node)) {
    } else if (const auto *child_node = dynamic_cast<const Parser::StringLiteral*>(node)) {
        std::string hash = "msg" + get_hash(8);
        segment_data.push_back(hash + " db \"" + child_node->value + "\", 0xd, 0xa, 0");
        entry_main.insert(entry_main.begin() + ix, "lea rcx, [" + hash + "]");
    } else if (const auto *n = dynamic_cast<const Parser::UnaryOperation*>(node)) {
    } else if (const auto *n = dynamic_cast<const Parser::BinaryOperation*>(node)) {
    } else {
        throw std::runtime_error("Unsupported node encountered in evaluation");
    }
}

void Compiler::evaluate_function_call(const Parser::FunctionCall *expr, size_t &ix) {
    if (expr->identifier == "println") {
        if (std::find(extern_fns.begin(), extern_fns.end(), "printf") == extern_fns.end()) {
            extern_fns.push_back("printf");
        }

        entry_main.insert(entry_main.begin() + ix, "call printf");

        evaluate_expr(expr->args[0].get(), ix);
    } else {
        throw std::runtime_error("Function signature: \"" + expr->identifier + "\" with " + std::to_string(expr->args.size()) + " args not defined");
    }
}

void Compiler::evaluate_unary_operation(const Parser::UnaryOperation *expr, size_t &ix) {
}

void Compiler::evaluate_binary_operation(const Parser::BinaryOperation *expr, size_t &ix) {
}

int Compiler::run_cmd(const std::string &cmd) {
    return system(cmd.c_str());
}

std::string Compiler::get_hash(const size_t &length) {
    const char legal_chars[] = "0123456789abcdef";

    std::random_device rd;
    std::mt19937 generator(rd());

    std::uniform_int_distribution<> distribution(0, sizeof(legal_chars) - 2);

    std::string res;

    for (size_t i = 0; i < length; ++i) {
        res += legal_chars[distribution(generator)];
    }

    return res;
}
