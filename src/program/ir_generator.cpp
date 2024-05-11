#include <program/ir_generator.h>

IRGenerator::IRGenerator(const Parser &parser) {
    success = true;
    generate_ir(parser.get());
}

void IRGenerator::generate_ir(const std::vector<std::unique_ptr<Parser::Node>> &ast) {
    for (const auto &t : ast) {
        evaluate_global_statement(t.get());
    }
}

void IRGenerator::evaluate_global_statement(const Parser::Node *statement) {
    if (const auto *decl = dynamic_cast<const Parser::FunctionDeclaration*>(statement)) {
        evaluate_function_declaration(decl);
    } else if (const auto *empty = dynamic_cast<const Parser::EmptyStatement*>(statement)) {
    } else {
        success = false;
        throw std::runtime_error("Unexpected global statement encountered.");
    }
}

void IRGenerator::evaluate_function_declaration(const Parser::FunctionDeclaration *decl) {
    std::string identifier = decl->identifier;
    if (identifier != "main") {
        identifier = decl->type + identifier;
        for (size_t i = 0; i < decl->args_ids.size(); ++i) {
            identifier += decl->args_types[i];
        }
        identifier = get_hash(identifier, "f");
    }

    auto entry = std::make_unique<Entry>(identifier);
    entry.get()->instructions.push_back(std::make_unique<Push>("rbp"));
    entry.get()->instructions.push_back(std::make_unique<Mov>("rbp", "rsp"));

    evaluate_wrapper_statement(decl->statement.get(), entry.get());

    entry.get()->instructions.push_back(std::make_unique<Xor>("rax", "rax"));
    entry.get()->instructions.push_back(std::make_unique<Leave>());
    entry.get()->instructions.push_back(std::make_unique<Ret>());

    text.declarations.push_back(std::move(entry));
}

void IRGenerator::evaluate_wrapper_statement(const Parser::Node *statement, Entry *entry) {
    int stack_size = 32;
    entry->instructions.push_back(std::make_unique<Sub>("rsp", std::to_string(stack_size + 32)));
    StackInfo stack_info;

    if (const auto *decl = dynamic_cast<const Parser::ScopeDeclaration*>(statement)) {
        for (const auto &t : decl->ast) {
            evaluate_statement(t.get(), entry, stack_info);
        }
    } else {
        evaluate_statement(statement, entry, stack_info);
    }
}

void IRGenerator::evaluate_statement(const Parser::Node *statement, Entry *entry, StackInfo &stack_info) {
    if (const auto *call = dynamic_cast<const Parser::FunctionCall*>(statement)) {
        evaluate_function_call(call, entry, stack_info);
    } else if (const auto *decl = dynamic_cast<const Parser::ScopeDeclaration*>(statement)) {
        int stack_size = 32;
        entry->instructions.push_back(std::make_unique<Sub>("rsp", std::to_string(stack_size + 32)));
        StackInfo nested_stack_info = stack_info;
        for (const auto &t : decl->ast) {
            evaluate_statement(t.get(), entry, nested_stack_info);
        }
    } else if (const auto *decl = dynamic_cast<const Parser::VariableDeclaration*>(statement)) {
        evaluate_variable_declaration(decl, entry, stack_info);
    } else if (const auto *empty = dynamic_cast<const Parser::EmptyStatement*>(statement)) {
    } else {
        success = false;
        throw std::runtime_error("Unexpected statement encountered.");
    }
}

void IRGenerator::evaluate_function_call(const Parser::FunctionCall *call, Entry *entry, StackInfo &stack_info) {
    if (call->identifier == "println") {
        for (int i = 0; i < call->args.size(); ++i) {
            const auto &arg = call->args[i];

            evaluate_expr(arg.get(), entry, "rcx", stack_info);

            add_extern("printf");
            entry->instructions.push_back(std::make_unique<Call>("printf"));
        }

        // Newline
        const std::string value = "0xd, 0xa";
        const std::string terminator = "0";
        const auto hash = get_hash(value + terminator, "c");
        push_unique(std::make_unique<Db>(hash, value, terminator), data);

        entry->instructions.push_back(std::make_unique<Lea>("rcx", "[" + hash + "]"));
        entry->instructions.push_back(std::make_unique<Call>("printf"));
    } else {
        std::string identifier = "void" + call->identifier;
        for (const auto &arg : call->args) {
            auto type_info = get_type_info(arg.get());
            identifier += type_info.name;
        }
        identifier = get_hash(identifier, "f");

        entry->instructions.push_back(std::make_unique<Call>(identifier));
    }
}

void IRGenerator::evaluate_variable_declaration(const Parser::VariableDeclaration *decl, Entry *entry, StackInfo &stack_info) {
    // const auto id = "static_" + decl->identifier;
    // push_unique(std::make_unique<Resd>(id, 1, decl->type), bss);
    if (!stack_info.exists(decl->identifier)) {
        evaluate_expr(decl->expr.get(), entry, "edx", stack_info);
        int offset = stack_info.get_offset();
        int size = 4;
        entry->instructions.push_back(std::make_unique<Mov>("dword [rbp - " + std::to_string(offset + size) +"]", "edx"));
        stack_info.push(decl->identifier, size);
    } else {
        success = false;
        throw std::runtime_error("Variable already declared: '" + decl->identifier + "'");
    }
}

void IRGenerator::evaluate_expr(const Parser::Node *expr, Entry *entry, const std::string &target, StackInfo &stack_info) {
    if (const auto *operation = dynamic_cast<const Parser::UnaryOperation*>(expr)) {
        evaluate_unary_operation(operation, entry, target, stack_info);
    } else if (const auto *operation = dynamic_cast<const Parser::BinaryOperation*>(expr)) {
        evaluate_binary_operation(operation, entry, target, stack_info);
    } else if (const auto *operation = dynamic_cast<const Parser::CastOperation*>(expr)) {
        const std::string type = operation->right;
        if (type == "string") {
            evaluate_expr(operation->left.get(), entry, "edx", stack_info);

            const std::string terminator = "0";
            const std::string value = "\"%d\"";
            const auto hash = get_hash(value + terminator, "c");
            push_unique(std::make_unique<Db>(hash, value, terminator), data);
            entry->instructions.push_back(std::make_unique<Lea>(target, "[" + hash + "]"));
        }
    } else if (const auto *call = dynamic_cast<const Parser::VariableCall*>(expr)) {
        if (stack_info.exists(call->identifier)) {
            entry->instructions.push_back(std::make_unique<Mov>(target, "[rbp - " + std::to_string(stack_info.get(call->identifier)) + "]"));
        } else {
            success = false;
            throw std::runtime_error("Variable not declared: '" + call->identifier + "'");
        }
    } else if (const auto *literal = dynamic_cast<const Parser::IntegerLiteral*>(expr)) {
        entry->instructions.push_back(std::make_unique<Mov>(target, std::to_string(literal->value)));
    } else if (const auto *literal = dynamic_cast<const Parser::StringLiteral*>(expr)) {
        const std::string value = '\"' + literal->value + '\"';
        const std::string terminator = "0";
        const auto hash = get_hash(value + terminator, "c");
        push_unique(std::make_unique<Db>(hash, value, terminator), data);
        entry->instructions.push_back(std::make_unique<Lea>(target, "[" + hash + "]"));
    } else {
        success = false;
        throw std::runtime_error("Unsupported expression encountered.");
    }
}

void IRGenerator::evaluate_binary_operation(const Parser::BinaryOperation *operation, Entry *entry, const std::string &target, StackInfo &stack_info) {
    std::string left_reg = "eax";
    std::string right_reg = "ebx";
    std::string temp_reg = "ecx";

    evaluate_expr(operation->left.get(), entry, left_reg, stack_info);

    if (dynamic_cast<Parser::BinaryOperation*>(operation->right.get())) {
        entry->instructions.push_back(std::make_unique<Mov>(temp_reg, left_reg));
        left_reg = temp_reg;
    }

    evaluate_expr(operation->right.get(), entry, right_reg, stack_info);

    if (operation->op == "+") {
        entry->instructions.push_back(std::make_unique<Add>(left_reg, right_reg));
    } else if (operation->op == "-") {
        entry->instructions.push_back(std::make_unique<Sub>(left_reg, right_reg));
    } else if (operation->op == "*") {
        entry->instructions.push_back(std::make_unique<Imul>(left_reg, right_reg));
    } else if (operation->op == "/") {
        entry->instructions.push_back(std::make_unique<Mov>("eax", left_reg));
        entry->instructions.push_back(std::make_unique<Xor>("edx", "edx"));
        entry->instructions.push_back(std::make_unique<Idiv>(right_reg));
        left_reg = "eax";
    }

    if (left_reg != target) {
        entry->instructions.push_back(std::make_unique<Mov>(target, left_reg));
    }
}

void IRGenerator::evaluate_unary_operation(const Parser::UnaryOperation *expr, Entry *entry, const std::string &target, StackInfo &stack_info) {
    if (expr->op == "-") {
        evaluate_expr(expr->value.get(), entry, target, stack_info);
        entry->instructions.push_back(std::make_unique<Imul>(target, "-1"));    
    } else {
        success = false;
        throw std::runtime_error("Unsupported operator: " + expr->op);
    }
}

void IRGenerator::push_unique(std::unique_ptr<Declaration> decl, Segment &target) {
    for (const auto &target_decl : target.declarations) {
        if (const auto *left = dynamic_cast<const Db*>(decl.get())) {
            if (const auto *right = dynamic_cast<const Db*>(target_decl.get())) {
                if (left->id == right->id && left->terminator == right->terminator) {
                    return;
                }
            }
        } else {
            if (decl->id == target_decl->id) {
                return;
            }
        }
    }

    target.declarations.push_back(std::move(decl));
}

void IRGenerator::add_extern(const std::string &id) {
    if (std::find(ext_libs.begin(), ext_libs.end(), id) == ext_libs.end()) {
        ext_libs.push_back(id);
    }
}

const IRGenerator::TypeInfo IRGenerator::get_type_info(const Parser::Node *expr) {
    TypeInfo type_info;
    if (const auto *literal = dynamic_cast<const Parser::IntegerLiteral*>(expr)) {
        type_info.name = "i32";
        type_info.type = IntegralType::INT;
        type_info.size = 4;
    } else if (const auto *literal = dynamic_cast<const Parser::StringLiteral*>(expr)) {
        type_info.name = "string";
        type_info.type = IntegralType::STRING;
        type_info.size = 24;
    } else if (const auto *call = dynamic_cast<const Parser::VariableCall*>(expr)) {
    } else if (const auto *operation = dynamic_cast<const Parser::UnaryOperation*>(expr)) {
    } else if (const auto *operation = dynamic_cast<const Parser::BinaryOperation*>(expr)) {
    } else if (const auto *operation = dynamic_cast<const Parser::CastOperation*>(expr)) {
    } else {
        success = false;
        throw std::runtime_error("Could not deduce type of unknown expression.");
    }
    return type_info;
}

const std::string IRGenerator::get_hash(const std::string &src, const std::string &prefix) const {
    unsigned long hash = 5381;
    for (const auto &c : src) {
        hash = ((hash << 5) + hash) + c;
    }
    std::stringstream ss;
    ss << std::hex << hash;
    return prefix + ss.str();
}

const bool IRGenerator::match_type(const std::string &id, const std::initializer_list<std::string> &types) const {
    for (const auto &decl : bss.declarations) {
        if (decl->id == get_hash(id)) {
            for (const auto &type : types) {
                if (decl->type == type) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool IRGenerator::StackInfo::exists(const std::string &id) {
    if (keys.find(id) == keys.end()) {
        return false;
    } else {
        return true;
    }
}

int IRGenerator::StackInfo::get(const std::string &id) {
    return keys.at(id);
}

int IRGenerator::StackInfo::get_offset() {
    if (keys.size() != 0) {
        return keys.rbegin()->second;
    } else {
        return 0;
    }
}

void IRGenerator::StackInfo::push(const std::string &id, const int &size) {
    keys.insert({id, get_offset() + size});
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

IRGenerator::Declaration::Declaration(const std::string &id, const std::string &type) : id(id), type(type) {}

void IRGenerator::Declaration::log() const {
    std::cout << "db: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "', type: '";
    std::cout << type;
    std::cout << "')" << '\n';
}

IRGenerator::Db::Db() {}

IRGenerator::Db::Db(const std::string &id, const std::string &value, const std::string &terminator) : Declaration(id, "string"), value(value), terminator(terminator) {}

void IRGenerator::Db::log() const {
    std::cout << "db: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "', type: '";
    std::cout << type;
    std::cout << "', value: '";
    std::cout << value;
    std::cout << "', terminator: '";
    std::cout << terminator;
    std::cout << "')" << '\n';
}

IRGenerator::Resb::Resb() {}

IRGenerator::Resb::Resb(const std::string &id, const int &fac, const std::string &type) : Declaration(id, type), fac(fac) {}

void IRGenerator::Resb::log() const {
    std::cout << "resb: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "', type: '";
    std::cout << type;
    std::cout << "', fac: '";
    std::cout << fac;
    std::cout << "')" << '\n';
}

IRGenerator::Resw::Resw() {}

IRGenerator::Resw::Resw(const std::string &id, const int &fac, const std::string &type) : Declaration(id, type), fac(fac) {}

void IRGenerator::Resw::log() const {
    std::cout << "resw: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "', type: '";
    std::cout << type;
    std::cout << "', fac: '";
    std::cout << fac;
    std::cout << "')" << '\n';
}

IRGenerator::Resd::Resd() {}

IRGenerator::Resd::Resd(const std::string &id, const int &fac, const std::string &type) : Declaration(id, type), fac(fac) {}

void IRGenerator::Resd::log() const {
    std::cout << "resd: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "', type: '";
    std::cout << type;
    std::cout << "', fac: '";
    std::cout << fac;
    std::cout << "')" << '\n';
}

IRGenerator::Resq::Resq() {}

IRGenerator::Resq::Resq(const std::string &id, const int &fac, const std::string &type) : Declaration(id, type), fac(fac) {}

void IRGenerator::Resq::log() const {
    std::cout << "resq: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "', type: '";
    std::cout << type;
    std::cout << "', fac: '";
    std::cout << fac;
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

IRGenerator::Leave::Leave() {}

void IRGenerator::Leave::log() const {
    std::cout << "leave" << '\n';
}

IRGenerator::Ret::Ret() {}

void IRGenerator::Ret::log() const {
    std::cout << "ret" << '\n';
}

IRGenerator::Call::Call() {}

IRGenerator::Call::Call(const std::string &id) : id(id) {}

void IRGenerator::Call::log() const {
    std::cout << "call: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "')" << '\n';
}