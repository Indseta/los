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
        for (size_t i = 0; i < decl->args_ids.size(); ++i) {
            identifier += decl->args_types[i];
        }
        identifier = get_hash(identifier, "f");
    }

    auto entry = std::make_unique<Entry>(identifier);
    entry->args_ids = decl->args_ids;
    entry.get()->instructions.push_back(std::make_unique<Push>("rbp"));
    entry.get()->instructions.push_back(std::make_unique<Mov>("rbp", "rsp"));

    evaluate_wrapper_statement(decl->statement.get(), entry.get());

    entry.get()->instructions.push_back(std::make_unique<Xor>("rax", "rax"));
    entry.get()->instructions.push_back(std::make_unique<Leave>());
    entry.get()->instructions.push_back(std::make_unique<Ret>());

    text.declarations.push_back(std::move(entry));
}

void IRGenerator::evaluate_wrapper_statement(const Parser::Node *statement, Entry *entry) {
    StackInfo stack_info;
    int alloc_at = entry->instructions.size();

    if (const auto *decl = dynamic_cast<const Parser::ScopeDeclaration*>(statement)) {
        for (const auto &t : decl->ast) {
            evaluate_statement(t.get(), entry, stack_info);
        }
    } else {
        evaluate_statement(statement, entry, stack_info);
    }

    stack_info.size = align_by(stack_info.size, 16);
    entry->instructions.insert(entry->instructions.begin() + alloc_at, std::make_unique<Sub>("rsp", std::to_string(stack_info.size + 32)));
}

void IRGenerator::evaluate_statement(const Parser::Node *statement, Entry *entry, StackInfo &stack_info) {
    if (const auto *call = dynamic_cast<const Parser::FunctionCall*>(statement)) {
        evaluate_function_call(call, entry, stack_info);
    } else if (const auto *decl = dynamic_cast<const Parser::ScopeDeclaration*>(statement)) {
        StackInfo nested_stack_info = stack_info;
        int alloc_at = entry->instructions.size();
        for (const auto &t : decl->ast) {
            evaluate_statement(t.get(), entry, nested_stack_info);
        }
        nested_stack_info.size = align_by(nested_stack_info.size, 16);
        entry->instructions.insert(entry->instructions.begin() + alloc_at, std::make_unique<Sub>("rsp", std::to_string(nested_stack_info.size)));
        entry->instructions.push_back(std::make_unique<Add>("rsp", std::to_string(nested_stack_info.size)));
    } else if (const auto *decl = dynamic_cast<const Parser::VariableDeclaration*>(statement)) {
        evaluate_variable_declaration(decl, entry, stack_info);
    } else if (const auto *assign = dynamic_cast<const Parser::VariableAssignment*>(statement)) {
        evaluate_variable_assignment(assign, entry, stack_info);
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
            // if (const auto *literal = dynamic_cast<const Parser::StringLiteral*>(arg.get())) {
            // } else {
            //     const auto operation = std::make_unique<Parser::CastOperation>(arg, "string");
            //     evaluate_expr(operation.get(), entry, "rcx", stack_info);
            // }

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
        std::string identifier = call->identifier;
        for (const auto &arg : call->args) {
            auto type_info = get_type_info(arg.get(), stack_info);
            identifier += type_info.name;
        }
        identifier = get_hash(identifier, "f");

        for (int i = 0; i < text.declarations.size(); ++i) {
            if (auto *decl = dynamic_cast<Entry*>(text.declarations[i].get())) {
                std::string other_id = decl->id;
                if (other_id == identifier) {
                    entry->instructions.push_back(std::make_unique<Call>(identifier));

                    for (int y = 0; y < decl->args_ids.size(); ++y) {
                        const auto &arg = decl->args_ids[y];
                        // decl->instructions.insert(decl->instructions.begin() + 3, std::make_unique<Mov>("rcx", "rcx"));
                        // evaluate_expr(call->args[y].get(), entry, "edx", stack_info);
                        // int offset = stack_info.get_offset();
                        // int size = 4;
                        // entry->instructions.push_back(std::make_unique<Mov>("dword [rbp - " + std::to_string(offset + size) +"]", "edx"));
                    }

                    return;
                }
            }
        }

        success = false;
        throw std::runtime_error("Function not declared or inaccessible: '" + call->identifier + "'");
    }
}

void IRGenerator::evaluate_variable_declaration(const Parser::VariableDeclaration *decl, Entry *entry, StackInfo &stack_info) {
    // const auto id = "static_" + decl->identifier;
    // push_unique(std::make_unique<Resd>(id, 1, decl->type), bss);
    if (!stack_info.exists(decl->identifier)) {
        int offset = stack_info.get_bottom();
        TypeInfo type_info = get_type_info(decl->type);
        stack_info.size += type_info.size;
        std::string registry = get_registry("rdx", type_info.size);
        evaluate_expr(decl->expr.get(), entry, registry, stack_info);
        entry->instructions.push_back(std::make_unique<Mov>(get_word(type_info.size) + " [rbp - " + std::to_string(offset) +"]", registry));
        stack_info.push(decl->identifier, type_info);
    } else {
        success = false;
        throw std::runtime_error("Variable already declared: '" + decl->identifier + "'");
    }
}

void IRGenerator::evaluate_variable_assignment(const Parser::VariableAssignment *assign, Entry *entry, StackInfo &stack_info) {
    if (stack_info.exists(assign->identifier)) {
        const auto &res = stack_info.get(assign->identifier);
        evaluate_expr(assign->expr.get(), entry, get_registry("rdx", res.type.size), stack_info);
        entry->instructions.push_back(std::make_unique<Mov>(get_word(res.type.size) + " [rbp - " + std::to_string(res.offset) +"]", get_registry("rdx", res.type.size)));
    } else {
        success = false;
        throw std::runtime_error("Variable not declared or inaccessible: '" + assign->identifier + "'");
    }
}

void IRGenerator::evaluate_expr(const Parser::Node *expr, Entry *entry, const std::string &target, StackInfo &stack_info) {
    if (const auto *operation = dynamic_cast<const Parser::UnaryOperation*>(expr)) {
        evaluate_unary_operation(operation, entry, target, stack_info);
    } else if (const auto *operation = dynamic_cast<const Parser::BinaryOperation*>(expr)) {
        evaluate_binary_operation(operation, entry, target, stack_info);
    } else if (const auto *operation = dynamic_cast<const Parser::CastOperation*>(expr)) {
        evaluate_cast_operation(operation, entry, target, stack_info);
    } else if (const auto *call = dynamic_cast<const Parser::VariableCall*>(expr)) {
        evaluate_variable_call(call, entry, target, stack_info);
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
    const auto type_info = get_type_info(operation, stack_info);
    std::string left_reg = get_registry("rax", type_info.size);
    std::string right_reg = get_registry("rbx", type_info.size);
    std::string temp_reg = get_registry("rcx", type_info.size);

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
        entry->instructions.push_back(std::make_unique<Mov>(get_registry("rax", type_info.size), left_reg));
        entry->instructions.push_back(std::make_unique<Xor>(get_registry("rdx", type_info.size), get_registry("rdx", type_info.size)));
        entry->instructions.push_back(std::make_unique<Idiv>(right_reg));
        left_reg = get_registry("rax", type_info.size);
    }

    if (left_reg != target) {
        entry->instructions.push_back(std::make_unique<Mov>(target, left_reg));
    }
}

void IRGenerator::evaluate_unary_operation(const Parser::UnaryOperation *operation, Entry *entry, const std::string &target, StackInfo &stack_info) {
    if (operation->op == "-") {
        evaluate_expr(operation->value.get(), entry, target, stack_info);
        entry->instructions.push_back(std::make_unique<Neg>(target));
    } else {
        success = false;
        throw std::runtime_error("Unsupported operator: " + operation->op);
    }
}

void IRGenerator::evaluate_cast_operation(const Parser::CastOperation *operation, Entry *entry, const std::string &target, StackInfo &stack_info) {
    const auto type_info = get_type_info(operation->left.get(), stack_info);
    const std::string cast_type = operation->right;
    if (cast_type == "string") {
        if (type_info.type == IntegralType::STRING) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        } else if (type_info.type == IntegralType::INT) {
            const std::string temp_reg = get_registry("rsi", type_info.size);
            evaluate_expr(operation->left.get(), entry, temp_reg, stack_info);
            entry->instructions.push_back(std::make_unique<Movsx>("rdx", temp_reg));

            const std::string terminator = "0";

            const std::string value = "\"%d\"";
            const auto hash = get_hash(value + terminator, "c");
            push_unique(std::make_unique<Db>(hash, value, terminator), data);
            entry->instructions.push_back(std::make_unique<Lea>(target, "[" + hash + "]"));
        } else if (type_info.type == IntegralType::UINT) {
            evaluate_expr(operation->left.get(), entry, get_registry("rdx", type_info.size), stack_info);

            const std::string terminator = "0";

            const std::string value = "\"%u\"";
            const auto hash = get_hash(value + terminator, "c");
            push_unique(std::make_unique<Db>(hash, value, terminator), data);
            entry->instructions.push_back(std::make_unique<Lea>(target, "[" + hash + "]"));
        }
    }
}

void IRGenerator::evaluate_variable_call(const Parser::VariableCall *call, Entry *entry, const std::string &target, StackInfo &stack_info) {
    if (stack_info.exists(call->identifier)) {
        entry->instructions.push_back(std::make_unique<Mov>(target, "[rbp - " + std::to_string(stack_info.get(call->identifier).offset) + "]"));
    } else {
        success = false;
        throw std::runtime_error("Variable not declared: '" + call->identifier + "'");
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

std::string IRGenerator::get_registry(const std::string &top_name, const int &size) {
    if (top_name == "rax") {
        if (size >= 8) return "rax";
        if (size >= 4) return "eax";
        if (size >= 2) return "ax";
        if (size >= 1) return "al";
    } else if (top_name == "rbx") {
        if (size >= 8) return "rbx";
        if (size >= 4) return "ebx";
        if (size >= 2) return "bx";
        if (size >= 1) return "bl";
    } else if (top_name == "rcx") {
        if (size >= 8) return "rcx";
        if (size >= 4) return "ecx";
        if (size >= 2) return "cx";
        if (size >= 1) return "cl";
    } else if (top_name == "rdx") {
        if (size >= 8) return "rdx";
        if (size >= 4) return "edx";
        if (size >= 2) return "dx";
        if (size >= 1) return "dl";
    } else if (top_name == "rsi") {
        if (size >= 8) return "rsi";
        if (size >= 4) return "esi";
        if (size >= 2) return "si";
        if (size >= 1) return "sil";
    } else if (top_name == "rdi") {
        if (size >= 8) return "rdi";
        if (size >= 4) return "edi";
        if (size >= 2) return "di";
        if (size >= 1) return "dil";
    } else if (top_name == "rbp") {
        if (size >= 8) return "rbp";
        if (size >= 4) return "ebp";
        if (size >= 2) return "bp";
        if (size >= 1) return "bpl";
    } else if (top_name == "rsp") {
        if (size >= 8) return "rsp";
        if (size >= 4) return "esp";
        if (size >= 2) return "sp";
        if (size >= 1) return "spl";
    } else if (top_name == "r8") {
        if (size >= 8) return "r8";
        if (size >= 4) return "r8d";
        if (size >= 2) return "r8w";
        if (size >= 1) return "r8b";
    } else if (top_name == "r9") {
        if (size >= 8) return "r9";
        if (size >= 4) return "r9d";
        if (size >= 2) return "r9w";
        if (size >= 1) return "r9b";
    } else if (top_name == "r10") {
        if (size >= 8) return "r10";
        if (size >= 4) return "r10d";
        if (size >= 2) return "r10w";
        if (size >= 1) return "r10b";
    } else if (top_name == "r11") {
        if (size >= 8) return "r11";
        if (size >= 4) return "r11d";
        if (size >= 2) return "r11w";
        if (size >= 1) return "r11b";
    } else if (top_name == "r12") {
        if (size >= 8) return "r12";
        if (size >= 4) return "r12d";
        if (size >= 2) return "r12w";
        if (size >= 1) return "r12b";
    } else if (top_name == "r13") {
        if (size >= 8) return "r13";
        if (size >= 4) return "r13d";
        if (size >= 2) return "r13w";
        if (size >= 1) return "r13b";
    } else if (top_name == "r14") {
        if (size >= 8) return "r14";
        if (size >= 4) return "r14d";
        if (size >= 2) return "r14w";
        if (size >= 1) return "r14b";
    } else if (top_name == "r15") {
        if (size >= 8) return "r15";
        if (size >= 4) return "r15d";
        if (size >= 2) return "r15w";
        if (size >= 1) return "r15b";
    }

    success = false;
    throw std::runtime_error("Could not deduce registry part of unknown top registry: '" + top_name + "', '" + std::to_string(size) + "' byte(s)");
}

std::string IRGenerator::get_word(const int &size) {
    if (size >= 8) return "qword";
    else if (size >= 4) return "dword";
    else if (size >= 2) return "word";
    else if (size >= 1) return "byte";
    else {
        success = false;
        throw std::runtime_error("Could not deduce word size: '" + std::to_string(size) + "' byte(s)");
    }
}

const IRGenerator::TypeInfo IRGenerator::get_type_info(const Parser::Node *expr, StackInfo &stack_info) {
    TypeInfo type_info;
    if (const auto *literal = dynamic_cast<const Parser::IntegerLiteral*>(expr)) {
        type_info.name = "i32";
        type_info.type = get_integral_type(type_info.name);
        type_info.size = get_data_size(type_info.name);
    } else if (const auto *literal = dynamic_cast<const Parser::StringLiteral*>(expr)) {
        type_info.name = "string";
        type_info.type = get_integral_type(type_info.name);
        type_info.size = get_data_size(type_info.name);
    } else if (const auto *call = dynamic_cast<const Parser::VariableCall*>(expr)) {
        type_info = stack_info.get(call->identifier).type;
    } else if (const auto *operation = dynamic_cast<const Parser::UnaryOperation*>(expr)) {
        const auto type_info = get_type_info(operation->value.get(), stack_info);
    } else if (const auto *operation = dynamic_cast<const Parser::BinaryOperation*>(expr)) {
        const auto left_type_info = get_type_info(operation->left.get(), stack_info);
        const auto right_type_info = get_type_info(operation->right.get(), stack_info);
        if (left_type_info.type == IntegralType::STRING) {
            if (right_type_info.type == IntegralType::STRING) {
                type_info = left_type_info;
                type_info.size = std::max(left_type_info.size, right_type_info.size);
                success = false;
            } else {
                success = false;
                throw std::runtime_error("Invalid expression: '" + left_type_info.name + "', '" + right_type_info.name + "'");
            }
        } else if (left_type_info.type == IntegralType::FLOAT || right_type_info.type == IntegralType::FLOAT) {
            type_info.name = left_type_info.name;
            type_info.type = get_integral_type(type_info.name);
            type_info.size = get_data_size(type_info.name);
        } else if (left_type_info.type == IntegralType::INT) {
            if (right_type_info.type == IntegralType::INT || right_type_info.type == IntegralType::UINT) {
                type_info.type = IntegralType::INT;
                type_info.size = std::max(left_type_info.size, right_type_info.size);
                type_info.name = "i" + std::to_string(type_info.size * 8);
            } else {
                success = false;
                throw std::runtime_error("Invalid expression: '" + left_type_info.name + "', '" + right_type_info.name + "'");
            }
        } else if (left_type_info.type == IntegralType::UINT) {
            if (right_type_info.type == IntegralType::INT) {
                type_info.type = IntegralType::INT;
                type_info.size = std::max(left_type_info.size, right_type_info.size);
                type_info.name = "i" + std::to_string(type_info.size * 8);
            } else if (right_type_info.type == IntegralType::UINT) {
                type_info.type = IntegralType::UINT;
                type_info.size = std::max(left_type_info.size, right_type_info.size);
                type_info.name = "u" + std::to_string(type_info.size * 8);
            } else {
                success = false;
                throw std::runtime_error("Invalid expression: '" + left_type_info.name + "', '" + right_type_info.name + "'");
            }
        }
    } else if (const auto *operation = dynamic_cast<const Parser::CastOperation*>(expr)) {
        type_info.name = operation->right;
        type_info.type = get_integral_type(type_info.name);
        type_info.size = get_data_size(type_info.name);
    } else {
        success = false;
        throw std::runtime_error("Could not deduce type of unknown expression.");
    }
    return type_info;
}

const IRGenerator::TypeInfo IRGenerator::get_type_info(const std::string &name) {
    TypeInfo type_info;
    type_info.name = name;
    type_info.type = get_integral_type(type_info.name);
    type_info.size = get_data_size(type_info.name);
    return type_info;
}

IRGenerator::IntegralType IRGenerator::get_integral_type(const std::string &name) {
    if (name == "string") return IntegralType::STRING;
    if (name == "bool") return IntegralType::BOOL;
    if (name == "i8") return IntegralType::INT;
    if (name == "i16") return IntegralType::INT;
    if (name == "i32") return IntegralType::INT;
    if (name == "i64") return IntegralType::INT;
    if (name == "u8") return IntegralType::UINT;
    if (name == "u16") return IntegralType::UINT;
    if (name == "u32") return IntegralType::UINT;
    if (name == "u64") return IntegralType::UINT;
    if (name == "f8") return IntegralType::FLOAT;
    if (name == "f16") return IntegralType::FLOAT;
    if (name == "f32") return IntegralType::FLOAT;
    if (name == "f64") return IntegralType::FLOAT;
    else return IntegralType::UNKNOWN;
}

int IRGenerator::get_data_size(const std::string &name) {
    if (name == "string") return 24;
    if (name == "bool") return 1;
    if (name == "i8") return 1;
    if (name == "i16") return 2;
    if (name == "i32") return 4;
    if (name == "i64") return 8;
    if (name == "u8") return 1;
    if (name == "u16") return 2;
    if (name == "u32") return 4;
    if (name == "u64") return 8;
    if (name == "f8") return 1;
    if (name == "f16") return 2;
    if (name == "f32") return 4;
    if (name == "f64") return 8;
    else {
        success = false;
        throw std::runtime_error("Could not deduce data size of unknown type: '" + name + "'");
    }
}

int IRGenerator::align_by(const int &src, const int &size) {
    if (src < 0) {
        std::cerr << "Alignment errror: " << src << '\n';
        return 0;
    } else if (src == 0) {
        return 0;
    } else {
        return ((src + size - 1) / size) * size;
    }
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

IRGenerator::TypeInfo::TypeInfo() : name("unknown"), type(IntegralType::UNKNOWN), size(0) {}

IRGenerator::StackEntry::StackEntry() : offset(0) {}

IRGenerator::StackInfo::StackInfo() : size(0) {}

IRGenerator::StackEntry IRGenerator::StackInfo::get(const std::string &id) {
    return keys.at(id);
}

int IRGenerator::StackInfo::get_bottom() {
    if (keys.size() != 0) {
        const auto &e = keys.rbegin()->second;
        return e.offset + e.type.size;
    } else {
        return 0;
    }
}

bool IRGenerator::StackInfo::exists(const std::string &id) {
    if (keys.find(id) == keys.end()) {
        return false;
    } else {
        return true;
    }
}

void IRGenerator::StackInfo::push(const std::string &id, const TypeInfo &type) {
    StackEntry entry;
    entry.offset = get_bottom();
    entry.type = type;
    keys.insert({id, entry});
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

IRGenerator::Movsx::Movsx() {}

IRGenerator::Movsx::Movsx(const std::string &dst, const std::string &src) : dst(dst), src(src) {}

void IRGenerator::Movsx::log() const {
    std::cout << "movsx: (";
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

IRGenerator::Neg::Neg() {}

IRGenerator::Neg::Neg(const std::string &dst) : dst(dst) {}

void IRGenerator::Neg::log() const {
    std::cout << "neg: (";
    std::cout << "dst: '";
    std::cout << dst;
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