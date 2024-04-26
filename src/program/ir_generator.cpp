#include <program/ir_generator.h>

// main:
//     push rbp
//     mov rbp, rsp
//     sub rsp, 32

//     mov eax, 50
//     mov ebx, 3
//     xor edx, edx
//     idiv ebx
//     mov edx, eax

//     lea rcx, [msg]
//     call printf

//     xor rcx, rcx
//     call ExitProcess

IRGenerator::IRGenerator(const Parser &parser) {
    success = true;
    generate_ir(parser.get());
}

void IRGenerator::generate_ir(const std::vector<std::unique_ptr<Parser::Node>> &ast) {
    for (const auto &t : ast) {
        evaluate_global_statement(t.get());
    }
}

void IRGenerator::evaluate_global_statement(const Parser::Node *expr) {
    if (const auto *node = dynamic_cast<const Parser::Entry*>(expr)) {
        evaluate_entry(node);
    } else if (const auto *node = dynamic_cast<const Parser::EmptyStatement*>(expr)) {
    } else {
        success = false;
        throw std::runtime_error("Unsupported statement encountered.");
    }
}

void IRGenerator::evaluate_entry(const Parser::Entry *expr) {
    auto entry = std::make_unique<Entry>("main");
    add_extern("ExitProcess");
    entry.get()->instructions.push_back(std::make_unique<Push>("rbp"));
    entry.get()->instructions.push_back(std::make_unique<Mov>("rbp", "rsp"));
    entry.get()->instructions.push_back(std::make_unique<Sub>("rsp", "32"));
    if (const auto *node = dynamic_cast<const Parser::ScopeDeclaration*>(expr->statement.get())) {
        for (const auto &t : node->ast) {
            evaluate_statement(t.get(), entry.get());
        }
    } else {
        evaluate_statement(expr->statement.get(), entry.get());
    }
    entry.get()->instructions.push_back(std::make_unique<Xor>("rcx", "rcx"));
    entry.get()->instructions.push_back(std::make_unique<Call>("ExitProcess"));
    text.declarations.push_back(std::move(entry));
}

void IRGenerator::evaluate_statement(const Parser::Node *expr, Entry *entry) {
    if (const auto *node = dynamic_cast<const Parser::FunctionCall*>(expr)) {
        evaluate_function_call(node, entry);
    } else if (const auto *node = dynamic_cast<const Parser::EmptyStatement*>(expr)) {
    } else {
        success = false;
        throw std::runtime_error("Unsupported statement encountered.");
    }
}

void IRGenerator::evaluate_function_call(const Parser::FunctionCall *expr, Entry *entry) {
    if (expr->identifier == "println") {
        if (const auto *node = dynamic_cast<const Parser::BinaryOperation*>(expr->args[0].get())) {
            const auto id = "data" + get_hash(16);
            data.declarations.push_back(std::make_unique<Db>(id, "\"%d\""));
            evaluate_expr(node, entry, "edx");
            entry->instructions.push_back(std::make_unique<Lea>("rcx", "[" + id + "]"));
        } else if (const auto *node = dynamic_cast<const Parser::StringLiteral*>(expr->args[0].get())) {
            evaluate_expr(node, entry, "rcx");
        } else if (const auto *node = dynamic_cast<const Parser::IntegerLiteral*>(expr->args[0].get())) {
            const auto id = "data" + get_hash(16);
            data.declarations.push_back(std::make_unique<Db>(id, "\"%d\""));
            evaluate_expr(node, entry, "edx");
            entry->instructions.push_back(std::make_unique<Lea>("rcx", "[" + id + "]"));
        } else if (const auto *node = dynamic_cast<const Parser::UnaryOperation*>(expr->args[0].get())) {
            const auto id = "data" + get_hash(16);
            data.declarations.push_back(std::make_unique<Db>(id, "\"%d\""));
            evaluate_expr(node, entry, "edx");
            entry->instructions.push_back(std::make_unique<Lea>("rcx", "[" + id + "]"));
        }
        add_extern("printf");
        entry->instructions.push_back(std::make_unique<Call>("printf"));
    } else {
        success = false;
        throw std::runtime_error("Function signature: \"" + expr->identifier + "\" with " + std::to_string(expr->args.size()) + " args not defined");
    }
}

void IRGenerator::evaluate_expr(const Parser::Node *expr, Entry *entry, const std::string &target) {
    if (const auto *operation = dynamic_cast<const Parser::UnaryOperation*>(expr)) {
        evaluate_unary_operation(operation, entry, target);
    } else if (const auto *operation = dynamic_cast<const Parser::BinaryOperation*>(expr)) {
        evaluate_binary_operation(operation, entry, target);
    } else if (const auto *literal = dynamic_cast<const Parser::IntegerLiteral*>(expr)) {
        entry->instructions.push_back(std::make_unique<Mov>(target, std::to_string(literal->value)));
    } else if (const auto *literal = dynamic_cast<const Parser::FloatLiteral*>(expr)) {
    } else if (const auto *literal = dynamic_cast<const Parser::BooleanLiteral*>(expr)) {
    } else if (const auto *literal = dynamic_cast<const Parser::StringLiteral*>(expr)) {
        const auto id = "data" + get_hash(16);
        data.declarations.push_back(std::make_unique<Db>(id, '\"' + literal->value + '\"'));
        entry->instructions.push_back(std::make_unique<Lea>(target, "[" + id + "]"));
    } else {
        success = false;
        throw std::runtime_error("Unsupported expression encountered.");
    }
}

void IRGenerator::evaluate_unary_operation(const Parser::UnaryOperation *expr, Entry *entry, const std::string &target) {
    if (expr->op == "-") {
        evaluate_expr(expr->value.get(), entry, target);
        entry->instructions.push_back(std::make_unique<Imul>(target, "-1"));
    } else {
        throw std::runtime_error("Unsupported operator: " + expr->op);
    }
}

void IRGenerator::evaluate_binary_operation(const Parser::BinaryOperation *expr, Entry *entry, const std::string &target) {
    if (const auto *left = dynamic_cast<const Parser::BinaryOperation*>(expr->left.get())) {
        evaluate_binary_operation(left, entry, "ebx");
    }
    if (const auto *right = dynamic_cast<const Parser::BinaryOperation*>(expr->right.get())) {
        evaluate_binary_operation(right, entry, target);
    }

    if (const auto *left = dynamic_cast<const Parser::UnaryOperation*>(expr->left.get())) {
        evaluate_unary_operation(left, entry, "ebx");
    }
    if (const auto *right = dynamic_cast<const Parser::UnaryOperation*>(expr->right.get())) {
        evaluate_unary_operation(right, entry, target);
    }

    if (const auto *left = dynamic_cast<const Parser::IntegerLiteral*>(expr->left.get())) {
        entry->instructions.push_back(std::make_unique<Mov>("ebx", std::to_string(left->value)));
    }
    if (const auto *right = dynamic_cast<const Parser::IntegerLiteral*>(expr->right.get())) {
        entry->instructions.push_back(std::make_unique<Mov>(target, std::to_string(right->value)));
    }

    if (expr->op == "*" || expr->op == "/") {
        if (expr->op == "*") {
            entry->instructions.push_back(std::make_unique<Imul>(target, "ebx"));
        } else {
            entry->instructions.push_back(std::make_unique<Mov>("eax", "ebx"));
            entry->instructions.push_back(std::make_unique<Mov>("ebx", target));
            entry->instructions.push_back(std::make_unique<Xor>("edx", "edx"));
            entry->instructions.push_back(std::make_unique<Idiv>("ebx"));
            entry->instructions.push_back(std::make_unique<Mov>(target, "eax"));
        }
    } else if (expr->op == "+" || expr->op == "-") {
        if (expr->op == "+") {
            entry->instructions.push_back(std::make_unique<Add>(target, "ebx"));
        } else {
            entry->instructions.push_back(std::make_unique<Sub>(target, "ebx"));
        }
    } else {
        throw std::runtime_error("Unsupported operator: " + expr->op);
    }
}

void IRGenerator::add_extern(const std::string &id) {
    if (std::find(ext_libs.begin(), ext_libs.end(), id) == ext_libs.end()) {
        ext_libs.push_back(id);
    }
}

std::string IRGenerator::get_hash(const size_t &length) {
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

const std::vector<std::string>& IRGenerator::get_ext_libs() const {
    return ext_libs;
}

const IRGenerator::Segment& IRGenerator::get_data() const {
    return data;
}

const IRGenerator::Segment& IRGenerator::get_bss() const {
    return bss;
}

const IRGenerator::Segment& IRGenerator::get_text() const {
    return text;
}

const bool& IRGenerator::get_success() const {
    return success;
}

void IRGenerator::log() const {
    std::cout << " -- IR result -- " << '\n';
    std::cout << "extern: (";
    for (int i = 0; i < ext_libs.size(); ++i) {
        const auto &l = ext_libs[i];
        std::cout << '\'' << l << '\'';
        if (i != ext_libs.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ")" << '\n';
    std::cout << '\n';
    std::cout << "segment .data" << '\n';
    for (const auto &n : data.declarations) {
        n->log();
    }
    std::cout << '\n';
    std::cout << "segment .bss" << '\n';
    for (const auto &n : bss.declarations) {
        n->log();
    }
    std::cout << '\n';
    std::cout << "segment .text" << '\n';
    for (const auto &n : text.declarations) {
        n->log();
    }
    std::cout << '\n';
}

IRGenerator::Statement::Statement() {}

void IRGenerator::Statement::log() const {
    std::cout << "statement: (none)" << '\n';
}

IRGenerator::Declaration::Declaration() {}

void IRGenerator::Declaration::log() const {
    std::cout << "declaration: (none)" << '\n';
}

IRGenerator::Db::Db() {}

IRGenerator::Db::Db(const std::string &id, const std::string &value) : id(id), value(value) {}

void IRGenerator::Db::log() const {
    std::cout << "db: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "', value: '";
    std::cout << value;
    std::cout << "')" << '\n';
}

IRGenerator::Segment::Segment() {}

void IRGenerator::Segment::log() const {
    std::cout << "segment: (";
    if (declarations.size() > 0) {
        std::cout << '\n';
        for (const auto &t : declarations) {
            std::cout << '\t';
            t->log();
        }
    }
    std::cout << ")" << '\n';
}

IRGenerator::Instruction::Instruction() {}

void IRGenerator::Instruction::log() const {
    std::cout << "instruction" << '\n';
}

IRGenerator::Entry::Entry() {}

IRGenerator::Entry::Entry(const std::string &id) : id(id) {}

void IRGenerator::Entry::log() const {
    std::cout << "entry: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "', instructions: (";
    if (instructions.size() > 0) {
        std::cout << '\n';
        for (const auto &t : instructions) {
            std::cout << '\t';
            t->log();
        }
    }
    std::cout << "))" << '\n';
}

IRGenerator::Push::Push() {}

IRGenerator::Push::Push(const std::string &src) : src(src) {}

void IRGenerator::Push::log() const {
    std::cout << "push: (";
    std::cout << "src: '";
    std::cout << src;
    std::cout << "')" << '\n';
}

IRGenerator::Mov::Mov() {}

IRGenerator::Mov::Mov(const std::string &dst, const std::string &src) : dst(dst), src(src) {}

void IRGenerator::Mov::log() const {
    std::cout << "mov: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "', src: '";
    std::cout << src;
    std::cout << "')" << '\n';
}

IRGenerator::Lea::Lea() {}

IRGenerator::Lea::Lea(const std::string &dst, const std::string &src) : dst(dst), src(src) {}

void IRGenerator::Lea::log() const {
    std::cout << "lea: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "', src: '";
    std::cout << src;
    std::cout << "')" << '\n';
}

IRGenerator::Imul::Imul() {}

IRGenerator::Imul::Imul(const std::string &dst, const std::string &src) : dst(dst), src(src) {}

void IRGenerator::Imul::log() const {
    std::cout << "imul: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "', src: '";
    std::cout << src;
    std::cout << "')" << '\n';
}

IRGenerator::Idiv::Idiv() {}

IRGenerator::Idiv::Idiv(const std::string &src) : src(src) {}

void IRGenerator::Idiv::log() const {
    std::cout << "idiv: (";
    std::cout << "src: '";
    std::cout << src;
    std::cout << "')" << '\n';
}

IRGenerator::Add::Add() {}

IRGenerator::Add::Add(const std::string &dst, const std::string &src) : dst(dst), src(src) {}

void IRGenerator::Add::log() const {
    std::cout << "add: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "', src: '";
    std::cout << src;
    std::cout << "')" << '\n';
}
IRGenerator::Sub::Sub() {}

IRGenerator::Sub::Sub(const std::string &dst, const std::string &src) : dst(dst), src(src) {}

void IRGenerator::Sub::log() const {
    std::cout << "sub: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "', src: '";
    std::cout << src;
    std::cout << "')" << '\n';
}

IRGenerator::Xor::Xor() {}

IRGenerator::Xor::Xor(const std::string &dst, const std::string &src) : dst(dst), src(src) {}

void IRGenerator::Xor::log() const {
    std::cout << "xor: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "', src: '";
    std::cout << src;
    std::cout << "')" << '\n';
}

IRGenerator::Call::Call() {}

IRGenerator::Call::Call(const std::string &id) : id(id) {}

void IRGenerator::Call::log() const {
    std::cout << "call: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "')" << '\n';
}