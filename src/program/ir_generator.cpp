#include <program/ir_generator.h>

IRGenerator::IRGenerator(const Parser &parser) {
    success = true;
    while_ix = 0;
    cnd_ix = 0;
    generate_ir(parser.get());
}

void IRGenerator::generate_ir(const std::vector<std::unique_ptr<Parser::Node>> &ast) {
    for (const auto &t : ast) {
        evaluate_global_statement(t.get());
    }
}

void IRGenerator::evaluate_global_statement(const Parser::Node *statement) {
    if (const auto *lib = dynamic_cast<const Parser::Extern*>(statement)) {
        std::cout << lib->id << '\n';
    } else if (const auto *mod = dynamic_cast<const Parser::Module*>(statement)) {
        evaluate_module(mod);
    } else if (const auto *decl = dynamic_cast<const Parser::FunctionDeclaration*>(statement)) {
        evaluate_function_declaration(decl);
    } else if (const auto *decl = dynamic_cast<const Parser::ClassDeclaration*>(statement)) {
        evaluate_class_declaration(decl);
    } else if (const auto *empty = dynamic_cast<const Parser::EmptyStatement*>(statement)) {
    } else {
        success = false;
        throw std::runtime_error("Unexpected global statement encountered.");
    }
}

void IRGenerator::evaluate_module(const Parser::Module *mod) {
    if (const auto *scope = dynamic_cast<const Parser::ScopeDeclaration*>(mod->statement.get())) {
        for (const auto &t : scope->ast) {
            evaluate_global_statement(t.get());
        }
    } else {
        evaluate_global_statement(mod->statement.get());
    }
}

void IRGenerator::evaluate_function_declaration(const Parser::FunctionDeclaration *decl) {
    std::string identifier = decl->identifier;
    bool is_main = identifier == "main" && decl->args_types.size() == 0;
    if (!is_main) {
        for (size_t i = 0; i < decl->args_ids.size(); ++i) {
            identifier += decl->args_types[i];
        }
        identifier = get_hash(identifier, "f");
    }

    auto entry = std::make_unique<Entry>(identifier);
    entry->type = decl->type;

    if (!is_main) {
        int offset = 16;
        for (size_t i = 0; i < decl->args_ids.size(); ++i) {
            StackEntry arg;
            arg.type = get_type_info(decl->args_types[i]);
            arg.offset = offset;
            offset += arg.type.size;
            entry->args_stack.keys.insert({decl->args_ids[i], arg});
        }
    }

    entry.get()->instructions.push_back(std::make_unique<Push>("rbp"));
    entry.get()->instructions.push_back(std::make_unique<Mov>("rbp", "rsp"));

    evaluate_wrapper_statement(decl->statement.get(), entry.get());

    if (is_main) entry.get()->instructions.push_back(std::make_unique<Xor>("rax", "rax"));
    entry.get()->instructions.push_back(std::make_unique<Jmp>("exit"));

    text.declarations.push_back(std::move(entry));
}

void IRGenerator::evaluate_class_declaration(const Parser::ClassDeclaration *decl) {
    auto class_info = std::make_unique<ClassInfo>(decl->identifier);

    std::string identifier = get_hash(decl->identifier, "f");

    auto declarator = std::make_unique<Entry>(identifier);
    declarator->type = "declarator";

    declarator->instructions.push_back(std::make_unique<Push>("rbp"));
    declarator->instructions.push_back(std::make_unique<Mov>("rbp", "rsp"));
    declarator->instructions.push_back(std::make_unique<Sub>("rsp", std::to_string(32)));

    declarator.get()->instructions.push_back(std::make_unique<Xor>("rax", "rax"));
    declarator.get()->instructions.push_back(std::make_unique<Jmp>("exit"));

    if (const auto *scope = dynamic_cast<const Parser::ScopeDeclaration*>(decl->statement.get())) {
        for (const auto &t : scope->ast) {
            evaluate_class_statement(t.get(), declarator.get(), class_info.get());
        }
    } else {
        evaluate_class_statement(decl->statement.get(), declarator.get(), class_info.get());
    }

    classes.insert({decl->identifier, std::move(class_info)});
}

void IRGenerator::evaluate_class_statement(const Parser::Node *statement, Entry *declarator, ClassInfo *class_info) {
    if (const auto *decl = dynamic_cast<const Parser::VariableDeclaration*>(statement)) {
        evaluate_variable_declaration(decl, declarator, class_info->stack);
    } else if (const auto *decl = dynamic_cast<const Parser::FunctionDeclaration*>(statement)) {
    } else {
        success = false;
        throw std::runtime_error("Unexpected class statement encountered.");
    }
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
    if (const auto *decl = dynamic_cast<const Parser::ScopeDeclaration*>(statement)) {
        StackInfo nested_stack_info = stack_info;
        int alloc_at = entry->instructions.size();
        for (const auto &t : decl->ast) {
            evaluate_statement(t.get(), entry, nested_stack_info);
        }
        nested_stack_info.size = align_by(nested_stack_info.size, 16);
        entry->instructions.insert(entry->instructions.begin() + alloc_at, std::make_unique<Sub>("rsp", std::to_string(nested_stack_info.size)));
        entry->instructions.push_back(std::make_unique<Add>("rsp", std::to_string(nested_stack_info.size)));
    } else if (const auto *exit = dynamic_cast<const Parser::ReturnStatement*>(statement)) {
        if (const auto *empty = dynamic_cast<const Parser::EmptyStatement*>(exit->expr.get())) {
            entry->instructions.push_back(std::make_unique<Jmp>("exit"));
        } else {
            const auto type_info = get_type_info(exit->expr.get(), entry, stack_info);
            const auto reg = get_registry("rax", type_info.size);
            entry->instructions.push_back(std::make_unique<Xor>("rax", "rax"));
            evaluate_expr(exit->expr.get(), entry, reg, stack_info);
            entry->instructions.push_back(std::make_unique<Jmp>("exit"));
        }
    } else if (const auto *loop = dynamic_cast<const Parser::WhileLoopStatement*>(statement)) {
        evaluate_while_statement(loop, entry, stack_info);
    } else if (const auto *cnd = dynamic_cast<const Parser::ConditionalStatement*>(statement)) {
        evaluate_conditional_statement(cnd, entry, stack_info);
    } else if (const auto *call = dynamic_cast<const Parser::FunctionCall*>(statement)) {
        evaluate_function_call(call, entry, "rax", stack_info);
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

void IRGenerator::evaluate_while_statement(const Parser::WhileLoopStatement *statement, Entry *entry, StackInfo &stack_info) {
    std::string idc = ".wlc" + std::to_string(while_ix);
    std::string idm = ".wlm" + std::to_string(while_ix);
    std::string ide = ".wle" + std::to_string(while_ix);

    entry->instructions.push_back(std::make_unique<Jmp>(idc));
    entry->instructions.push_back(std::make_unique<Label>(ide));


    auto wlc = std::make_unique<Entry>(idc);
    wlc->type = "void";

    const std::string reg = get_registry("rcx", get_type_info(statement->condition.get(), wlc.get(), stack_info).size);
    evaluate_expr(statement->condition.get(), wlc.get(), reg, stack_info);
    wlc->instructions.push_back(std::make_unique<Cmp>(reg, "1"));
    wlc->instructions.push_back(std::make_unique<Je>(idm));
    wlc->instructions.push_back(std::make_unique<Jne>(ide));

    auto wlm = std::make_unique<Entry>(idm);
    wlm->type = "void";

    StackInfo nested_stack_info = stack_info;
    int alloc_at = wlm->instructions.size();

    if (const auto *decl = dynamic_cast<const Parser::ScopeDeclaration*>(statement->statement.get())) {
        for (const auto &t : decl->ast) {
            evaluate_statement(t.get(), wlm.get(), nested_stack_info);
        }
    } else {
        evaluate_statement(statement->statement.get(), wlm.get(), nested_stack_info);
    }

    nested_stack_info.size = align_by(nested_stack_info.size, 16);
    wlm->instructions.insert(wlm->instructions.begin() + alloc_at, std::make_unique<Sub>("rsp", std::to_string(nested_stack_info.size)));
    wlm->instructions.push_back(std::make_unique<Add>("rsp", std::to_string(nested_stack_info.size)));
    wlm->instructions.push_back(std::make_unique<Jmp>(idc));

    labels.push_back(std::move(wlc));
    labels.push_back(std::move(wlm));

    ++while_ix;
}

void IRGenerator::evaluate_conditional_statement(const Parser::ConditionalStatement *statement, Entry *entry, StackInfo &stack_info) {
    const std::string reg = get_registry("rcx", get_type_info(statement->condition.get(), entry, stack_info).size);
    evaluate_expr(statement->condition.get(), entry, reg, stack_info);

    entry->instructions.push_back(std::make_unique<Cmp>(reg, "1"));

    std::string idm = ".cndm" + std::to_string(cnd_ix);
    std::string ide = ".cnde" + std::to_string(cnd_ix);
    ++cnd_ix;

    entry->instructions.push_back(std::make_unique<Je>(idm));
    auto cndm = std::make_unique<Entry>(idm);
    cndm->type = "void";

    StackInfo pass_stack_info = stack_info;
    int alloc_at = cndm->instructions.size();
    if (const auto *decl = dynamic_cast<const Parser::ScopeDeclaration*>(statement->pass_statement.get())) {
        for (const auto &t : decl->ast) {
            evaluate_statement(t.get(), cndm.get(), pass_stack_info);
        }
    } else {
        evaluate_statement(statement->pass_statement.get(), cndm.get(), pass_stack_info);
    }
    pass_stack_info.size = align_by(pass_stack_info.size, 16);
    cndm->instructions.insert(cndm->instructions.begin() + alloc_at, std::make_unique<Sub>("rsp", std::to_string(pass_stack_info.size)));
    cndm->instructions.push_back(std::make_unique<Add>("rsp", std::to_string(pass_stack_info.size)));

    cndm.get()->instructions.push_back(std::make_unique<Jmp>(ide));
    labels.push_back(std::move(cndm));

    if (const auto *empty = dynamic_cast<const Parser::EmptyStatement*>(statement->fail_statement.get())) {
    } else {
        idm = ".cndm" + std::to_string(cnd_ix);
        ++cnd_ix;

        entry->instructions.push_back(std::make_unique<Jne>(idm));
        auto cndms = std::make_unique<Entry>(idm);
        cndms->type = "void";

        StackInfo fail_stack_info = stack_info;
        int alloc_at = cndms->instructions.size();
        if (const auto *decl = dynamic_cast<const Parser::ScopeDeclaration*>(statement->fail_statement.get())) {
            for (const auto &t : decl->ast) {
                evaluate_statement(t.get(), cndms.get(), fail_stack_info);
            }
        } else {
            evaluate_statement(statement->fail_statement.get(), cndms.get(), fail_stack_info);
        }
        fail_stack_info.size = align_by(fail_stack_info.size, 16);
        cndms->instructions.insert(cndms->instructions.begin() + alloc_at, std::make_unique<Sub>("rsp", std::to_string(fail_stack_info.size)));
        cndms->instructions.push_back(std::make_unique<Add>("rsp", std::to_string(fail_stack_info.size)));

        cndms.get()->instructions.push_back(std::make_unique<Jmp>(ide));
        labels.push_back(std::move(cndms));
    }

    entry->instructions.push_back(std::make_unique<Label>(ide));
}

void IRGenerator::evaluate_function_call(const Parser::FunctionCall *call, Entry *entry, const std::string &target, StackInfo &stack_info) {
    if (call->identifier == "printf") {
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
        std::string identifier = call->identifier;
        for (const auto &arg : call->args) {
            auto type_info = get_type_info(arg.get(), entry, stack_info);
            identifier += type_info.name;
        }
        identifier = get_hash(identifier, "f");

        for (int i = 0; i < text.declarations.size(); ++i) {
            if (auto *decl = dynamic_cast<Entry*>(text.declarations[i].get())) {
                std::string other_id = decl->id;
                std::string other_type = decl->type;
                if (other_id == identifier) {
                    int alloc_at = entry->instructions.size();
                    int offset = 0;

                    int y = 0;
                    for (const auto &decl_arg : decl->args_stack.keys) {
                        const auto &call_arg = call->args[y].get();
                        const std::string temp_reg = get_registry("rsi", decl_arg.second.type.size);
                        evaluate_expr(call_arg, entry, temp_reg, stack_info);
                        entry->instructions.push_back(std::make_unique<Mov>(get_word(decl_arg.second.type.size) + " [rsp + " + std::to_string(offset) + "]", temp_reg));

                        offset += decl_arg.second.type.size;
                        ++y;
                    }

                    const std::string res_size = std::to_string(align_by(offset, 16));
                    entry->instructions.insert(entry->instructions.begin() + alloc_at, std::make_unique<Sub>("rsp", res_size));
                    entry->instructions.push_back(std::make_unique<Call>(identifier));
                    entry->instructions.push_back(std::make_unique<Add>("rsp", res_size));
                    if (decl->type != "void") entry->instructions.push_back(std::make_unique<Mov>(target, get_registry("rax", get_data_size(decl->type))));
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
    if (is_integral(decl->type)) {
        if (!stack_info.exists(decl->identifier)) {
            int offset = stack_info.get_bottom();
            TypeInfo type_info = get_type_info(decl->type);
            stack_info.size += type_info.size;
            std::string registry = get_registry("rdx", type_info.size);
            if (const auto *statement = dynamic_cast<const Parser::EmptyStatement*>(decl->expr.get())) {
                if (type_info.type == IntegralType::BOOL) {
                    entry->instructions.push_back(std::make_unique<Mov>(get_word(type_info.size) + " [rbp - " + std::to_string(offset) +"]", "0"));
                } else if (type_info.type == IntegralType::UINT) {
                    entry->instructions.push_back(std::make_unique<Mov>(get_word(type_info.size) + " [rbp - " + std::to_string(offset) +"]", "0"));
                } else if (type_info.type == IntegralType::INT) {
                    entry->instructions.push_back(std::make_unique<Mov>(get_word(type_info.size) + " [rbp - " + std::to_string(offset) +"]", "0"));
                } else if (type_info.type == IntegralType::STRING) {
                    entry->instructions.push_back(std::make_unique<Mov>(get_word(type_info.size) + " [rbp - " + std::to_string(offset) +"]", "Uninitialized string"));
                }
            } else {
                evaluate_expr(decl->expr.get(), entry, registry, stack_info);
                entry->instructions.push_back(std::make_unique<Mov>(get_word(type_info.size) + " [rbp - " + std::to_string(offset) +"]", registry));
            }
            stack_info.push(decl->identifier, type_info);
        } else {
            success = false;
            throw std::runtime_error("Variable already declared: '" + decl->identifier + "'");
        }
    } else if (is_class(decl->type)) {
        if (!stack_info.exists(decl->identifier)) {
            int offset = stack_info.get_bottom();
            TypeInfo type_info = get_type_info(decl->type);
            stack_info.size += type_info.size;

            stack_info.push(decl->identifier, type_info);
        } else {
            success = false;
            throw std::runtime_error("Variable already declared: '" + decl->identifier + "'");
        }
    } else {
        success = false;
        throw std::runtime_error("Could not evaluate unknown declaration: '" + decl->identifier + "'");
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
    } else if (const auto *call = dynamic_cast<const Parser::FunctionCall*>(expr)) {
        evaluate_function_call(call, entry, target, stack_info);
    } else if (const auto *literal = dynamic_cast<const Parser::IntegerLiteral*>(expr)) {
        entry->instructions.push_back(std::make_unique<Mov>(target, literal->value));
    } else if (const auto *literal = dynamic_cast<const Parser::BooleanLiteral*>(expr)) {
        entry->instructions.push_back(std::make_unique<Mov>(target, std::to_string(literal->value)));
    } else if (const auto *literal = dynamic_cast<const Parser::StringLiteral*>(expr)) {
        const std::string value = '\"' + literal->value + '\"';
        const std::string terminator = "0";
        const auto hash = get_hash(value + terminator, "c");
        push_unique(std::make_unique<Db>(hash, value, terminator), data);
        entry->instructions.push_back(std::make_unique<Lea>(target, "[" + hash + "]"));
    } else if (const auto *literal = dynamic_cast<const Parser::EmptyStatement*>(expr)) {
    } else {
        success = false;
        throw std::runtime_error("Unsupported expression encountered.");
    }
}

void IRGenerator::evaluate_binary_operation(const Parser::BinaryOperation *operation, Entry *entry, const std::string &target, StackInfo &stack_info) {
    auto left_type = get_type_info(operation->left.get(), entry, stack_info);
    auto right_type = get_type_info(operation->left.get(), entry, stack_info);
    std::string left_reg = get_registry("rax", left_type.size);
    std::string right_reg = get_registry("rbx", right_type.size);
    std::string temp_reg = get_registry("rcx", right_type.size);

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
        entry->instructions.push_back(std::make_unique<Mov>(get_registry("rax", left_type.size), left_reg));
        entry->instructions.push_back(std::make_unique<Xor>("rdx", "rdx"));
        entry->instructions.push_back(std::make_unique<Idiv>(right_reg));
        left_reg = get_registry("rax", left_type.size);
    } else if (operation->op == "==") {
        entry->instructions.push_back(std::make_unique<Cmp>(left_reg, right_reg));
        entry->instructions.push_back(std::make_unique<Sete>(get_registry(left_reg, get_type_info("bool").size)));
    } else if (operation->op == "!=") {
        entry->instructions.push_back(std::make_unique<Cmp>(left_reg, right_reg));
        entry->instructions.push_back(std::make_unique<Setne>(get_registry(left_reg, get_type_info("bool").size)));
    } else if (operation->op == ">") {
        entry->instructions.push_back(std::make_unique<Cmp>(left_reg, right_reg));
        entry->instructions.push_back(std::make_unique<Setg>(get_registry(left_reg, get_type_info("bool").size)));
    } else if (operation->op == ">=") {
        entry->instructions.push_back(std::make_unique<Cmp>(left_reg, right_reg));
        entry->instructions.push_back(std::make_unique<Setge>(get_registry(left_reg, get_type_info("bool").size)));
    } else if (operation->op == "<") {
        entry->instructions.push_back(std::make_unique<Cmp>(left_reg, right_reg));
        entry->instructions.push_back(std::make_unique<Setl>(get_registry(left_reg, get_type_info("bool").size)));
    } else if (operation->op == "<=") {
        entry->instructions.push_back(std::make_unique<Cmp>(left_reg, right_reg));
        entry->instructions.push_back(std::make_unique<Setle>(get_registry(left_reg, get_type_info("bool").size)));
    }

    if (left_reg != target) {
        entry->instructions.push_back(std::make_unique<Mov>(get_registry(target, left_type.size), left_reg));
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
    const auto org_type = get_type_info(operation->left.get(), entry, stack_info);
    const auto cast_type = get_type_info(operation->right);
    if (cast_type.type == IntegralType::BOOL) {
        if (org_type.type == IntegralType::STRING) {
        } else if (org_type.type == IntegralType::INT) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        } else if (org_type.type == IntegralType::UINT) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        } else if (org_type.type == IntegralType::BOOL) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        }
    } else if (cast_type.type == IntegralType::UINT) {
        if (org_type.type == IntegralType::STRING) {
        } else if (org_type.type == IntegralType::INT) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        } else if (org_type.type == IntegralType::UINT) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        } else if (org_type.type == IntegralType::BOOL) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        }
    } else if (cast_type.type == IntegralType::INT) {
        if (org_type.type == IntegralType::STRING) {
        } else if (org_type.type == IntegralType::INT) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        } else if (org_type.type == IntegralType::UINT) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        } else if (org_type.type == IntegralType::BOOL) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        }
    } else if (cast_type.type == IntegralType::STRING) {
        if (org_type.type == IntegralType::STRING) {
            evaluate_expr(operation->left.get(), entry, target, stack_info);
        } else if (org_type.type == IntegralType::INT) {
            const std::string temp_reg = get_registry("rsi", org_type.size);
            evaluate_expr(operation->left.get(), entry, temp_reg, stack_info);
            if (org_type.size < 8) entry->instructions.push_back(std::make_unique<Movsx>("rdx", temp_reg));
            else entry->instructions.push_back(std::make_unique<Mov>("rdx", temp_reg));

            const std::string value = org_type.size >= 8 ? "\"%lld\"" : "\"%d\"";
            const std::string terminator = "0";

            const auto hash = get_hash(value + terminator, "c");
            push_unique(std::make_unique<Db>(hash, value, terminator), data);
            entry->instructions.push_back(std::make_unique<Lea>(target, "[" + hash + "]"));
        } else if (org_type.type == IntegralType::UINT) {
            std::string temp_reg = get_registry("rsi", org_type.size);
            evaluate_expr(operation->left.get(), entry, temp_reg, stack_info);
            entry->instructions.push_back(std::make_unique<Mov>("rdx", "rsi"));

            const std::string value = org_type.size >= 8 ? "\"%llu\"" : "\"%u\"";
            const std::string terminator = "0";

            const auto hash = get_hash(value + terminator, "c");
            push_unique(std::make_unique<Db>(hash, value, terminator), data);
            entry->instructions.push_back(std::make_unique<Lea>(target, "[" + hash + "]"));
        } else if (org_type.type == IntegralType::BOOL) {
            const std::string true_value = "\"true\"";
            const std::string false_value = "\"false\"";
            const std::string terminator = "0";

            const auto true_hash = get_hash(true_value + terminator, "c");
            const auto false_hash = get_hash(false_value + terminator, "c");
            push_unique(std::make_unique<Db>(true_hash, true_value, terminator), data);
            push_unique(std::make_unique<Db>(false_hash, false_value, terminator), data);

            const std::string temp_reg = get_registry("rsi", org_type.size);

            evaluate_expr(operation->left.get(), entry, temp_reg, stack_info);

            entry->instructions.push_back(std::make_unique<Lea>("rdx", "[" + true_hash + "]"));
            entry->instructions.push_back(std::make_unique<Lea>(target, "[" + false_hash + "]"));

            entry->instructions.push_back(std::make_unique<Cmp>(temp_reg, "1"));
            entry->instructions.push_back(std::make_unique<Cmove>(target, "rdx"));
        }
    }
}

void IRGenerator::evaluate_variable_call(const Parser::VariableCall *call, Entry *entry, const std::string &target, StackInfo &stack_info) {
    if (stack_info.exists(call->identifier)) {
        entry->instructions.push_back(std::make_unique<Mov>(target, "[rbp - " + std::to_string(stack_info.get(call->identifier).offset) + "]"));
    } else if (entry->args_stack.exists(call->identifier)) {
        entry->instructions.push_back(std::make_unique<Mov>(target, "[rbp + " + std::to_string(entry->args_stack.get(call->identifier).offset) + "]"));
    } else {
        success = false;
        throw std::runtime_error("Variable not declared or inaccessible: '" + call->identifier + "'");
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
    if (top_name == "rax" || top_name == "eax" || top_name == "ax" || top_name == "al") {
        if (size >= 8) return "rax";
        if (size >= 4) return "eax";
        if (size >= 2) return "ax";
        if (size >= 1) return "al";
    } else if (top_name == "rbx" || top_name == "ebx" || top_name == "bx" || top_name == "bl") {
        if (size >= 8) return "rbx";
        if (size >= 4) return "ebx";
        if (size >= 2) return "bx";
        if (size >= 1) return "bl";
    } else if (top_name == "rcx" || top_name == "ecx" || top_name == "cx" || top_name == "cl") {
        if (size >= 8) return "rcx";
        if (size >= 4) return "ecx";
        if (size >= 2) return "cx";
        if (size >= 1) return "cl";
    } else if (top_name == "rdx" || top_name == "edx" || top_name == "dx" || top_name == "dl") {
        if (size >= 8) return "rdx";
        if (size >= 4) return "edx";
        if (size >= 2) return "dx";
        if (size >= 1) return "dl";
    } else if (top_name == "rsi" || top_name == "esi" || top_name == "si" || top_name == "sil") {
        if (size >= 8) return "rsi";
        if (size >= 4) return "esi";
        if (size >= 2) return "si";
        if (size >= 1) return "sil";
    } else if (top_name == "rdi" || top_name == "edi" || top_name == "di" || top_name == "dil") {
        if (size >= 8) return "rdi";
        if (size >= 4) return "edi";
        if (size >= 2) return "di";
        if (size >= 1) return "dil";
    } else if (top_name == "rbp" || top_name == "ebp" || top_name == "bp" || top_name == "bpl") {
        if (size >= 8) return "rbp";
        if (size >= 4) return "ebp";
        if (size >= 2) return "bp";
        if (size >= 1) return "bpl";
    } else if (top_name == "rsp" || top_name == "esp" || top_name == "sp" || top_name == "spl") {
        if (size >= 8) return "rsp";
        if (size >= 4) return "esp";
        if (size >= 2) return "sp";
        if (size >= 1) return "spl";
    } else if (top_name == "r8" || top_name == "r8d" || top_name == "r8w" || top_name == "r8b") {
        if (size >= 8) return "r8";
        if (size >= 4) return "r8d";
        if (size >= 2) return "r8w";
        if (size >= 1) return "r8b";
    } else if (top_name == "r9" || top_name == "r9d" || top_name == "r9w" || top_name == "r9b") {
        if (size >= 8) return "r9";
        if (size >= 4) return "r9d";
        if (size >= 2) return "r9w";
        if (size >= 1) return "r9b";
    } else if (top_name == "r10" || top_name == "r10d" || top_name == "r10w" || top_name == "r10b") {
        if (size >= 8) return "r10";
        if (size >= 4) return "r10d";
        if (size >= 2) return "r10w";
        if (size >= 1) return "r10b";
    } else if (top_name == "r11" || top_name == "r11d" || top_name == "r11w" || top_name == "r11b") {
        if (size >= 8) return "r11";
        if (size >= 4) return "r11d";
        if (size >= 2) return "r11w";
        if (size >= 1) return "r11b";
    } else if (top_name == "r12" || top_name == "r12d" || top_name == "r12w" || top_name == "r12b") {
        if (size >= 8) return "r12";
        if (size >= 4) return "r12d";
        if (size >= 2) return "r12w";
        if (size >= 1) return "r12b";
    } else if (top_name == "r13" || top_name == "r13d" || top_name == "r13w" || top_name == "r13b") {
        if (size >= 8) return "r13";
        if (size >= 4) return "r13d";
        if (size >= 2) return "r13w";
        if (size >= 1) return "r13b";
    } else if (top_name == "r14" || top_name == "r14d" || top_name == "r14w" || top_name == "r14b") {
        if (size >= 8) return "r14";
        if (size >= 4) return "r14d";
        if (size >= 2) return "r14w";
        if (size >= 1) return "r14b";
    } else if (top_name == "r15" || top_name == "r15d" || top_name == "r15w" || top_name == "r15b") {
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

const bool IRGenerator::is_class(const std::string &name) {
    if (!is_integral(name)) {
        if (classes.find(name) != classes.end()) return true;
    }

    return false;
}

const bool IRGenerator::is_integral(const std::string &name) {
    const auto integral = get_integral_type(name);
    if (integral == UNKNOWN) return false;
    else return true;
}

const IRGenerator::TypeInfo IRGenerator::get_type_info(const Parser::Node *expr, Entry *entry, StackInfo &stack_info) {
    TypeInfo type_info;
    if (const auto *literal = dynamic_cast<const Parser::IntegerLiteral*>(expr)) {
        type_info.name = "int32";
        type_info.type = get_integral_type(type_info.name);
        type_info.size = get_data_size(type_info.name);
    } else if (const auto *literal = dynamic_cast<const Parser::BooleanLiteral*>(expr)) {
        type_info.name = "bool";
        type_info.type = get_integral_type(type_info.name);
        type_info.size = get_data_size(type_info.name);
    } else if (const auto *literal = dynamic_cast<const Parser::StringLiteral*>(expr)) {
        type_info.name = "string";
        type_info.type = get_integral_type(type_info.name);
        type_info.size = get_data_size(type_info.name);
    } else if (const auto *call = dynamic_cast<const Parser::VariableCall*>(expr)) {
        if (stack_info.exists(call->identifier)) {
            type_info = stack_info.get(call->identifier).type;
        } else if (entry->args_stack.exists(call->identifier)) {
            type_info = entry->args_stack.get(call->identifier).type;
        } else {
            success = false;
            throw std::runtime_error("Variable not declared or inaccessible: '" + call->identifier + "'");
        }
    } else if (const auto *call = dynamic_cast<const Parser::FunctionCall*>(expr)) {
        std::string identifier = call->identifier;
        for (const auto &arg : call->args) {
            auto type_info = get_type_info(arg.get(), entry, stack_info);
            identifier += type_info.name;
        }
        identifier = get_hash(identifier, "f");
        for (int i = 0; i < text.declarations.size(); ++i) {
            if (auto *decl = dynamic_cast<Entry*>(text.declarations[i].get())) {
                if (decl->id == identifier) {
                    type_info = get_type_info(decl->type);
                }
            }
        }
    } else if (const auto *operation = dynamic_cast<const Parser::UnaryOperation*>(expr)) {
        type_info = get_type_info(operation->value.get(), entry, stack_info);
    } else if (const auto *operation = dynamic_cast<const Parser::BinaryOperation*>(expr)) {
        const auto left_type_info = get_type_info(operation->left.get(), entry, stack_info);
        const auto right_type_info = get_type_info(operation->right.get(), entry, stack_info);
        if (operation->op == "==" || operation->op == "!=" || operation->op == ">" || operation->op == ">=" || operation->op == "<" || operation->op == "<=") {
            type_info.name = "bool";
            type_info.type = get_integral_type(type_info.name);
            type_info.size = get_data_size(type_info.name);
        } else {
            if (left_type_info.type == IntegralType::STRING) {
                if (right_type_info.type == IntegralType::STRING) {
                    type_info = left_type_info;
                    type_info.size = std::max(left_type_info.size, right_type_info.size);
                    success = false;
                    throw std::runtime_error("Strinc concatenation not supported.");
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
    if (is_class(name)) {
        const auto &c = classes.at(name);
        TypeInfo type_info;
        type_info.name = name;
        type_info.type = UNKNOWN;
        type_info.size = c->stack.size;
        return type_info;
    } else if (is_integral(name)) {
        TypeInfo type_info;
        type_info.name = name;
        type_info.type = get_integral_type(type_info.name);
        type_info.size = get_data_size(type_info.name);
        return type_info;
    } else {
        success = false;
        throw std::runtime_error("Could not deduce type of unknown variable.");
    }
}

IRGenerator::IntegralType IRGenerator::get_integral_type(const std::string &name) {
    if (name == "string") return IntegralType::STRING;
    if (name == "bool") return IntegralType::BOOL;
    if (name == "int8") return IntegralType::INT;
    if (name == "int16") return IntegralType::INT;
    if (name == "int32") return IntegralType::INT;
    if (name == "int64") return IntegralType::INT;
    if (name == "uint8") return IntegralType::UINT;
    if (name == "uint16") return IntegralType::UINT;
    if (name == "uint32") return IntegralType::UINT;
    if (name == "uint64") return IntegralType::UINT;
    if (name == "float8") return IntegralType::FLOAT;
    if (name == "float16") return IntegralType::FLOAT;
    if (name == "float32") return IntegralType::FLOAT;
    if (name == "float64") return IntegralType::FLOAT;
    else return IntegralType::UNKNOWN;
}

int IRGenerator::get_data_size(const std::string &name) {
    if (name == "string") return 8;
    if (name == "bool") return 1;
    if (name == "int8") return 1;
    if (name == "int16") return 2;
    if (name == "int32") return 4;
    if (name == "int64") return 8;
    if (name == "uint8") return 1;
    if (name == "uint16") return 2;
    if (name == "uint32") return 4;
    if (name == "uint64") return 8;
    if (name == "float8") return 1;
    if (name == "float16") return 2;
    if (name == "float32") return 4;
    if (name == "float64") return 8;
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

const std::vector<std::unique_ptr<IRGenerator::Declaration>>& IRGenerator::get_labels() const {
    return labels;
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
    for (const auto &n : labels) {
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

IRGenerator::Cmp::Cmp() {}

IRGenerator::Cmp::Cmp(const std::string &left, const std::string &right) : left(left), right(right) {}

void IRGenerator::Cmp::log() const {
    std::cout << "cmp: (";
    std::cout << "left: '";
    std::cout << left;
    std::cout << "', right: '";
    std::cout << right;
    std::cout << "')" << '\n';
}

IRGenerator::Sete::Sete() {}

IRGenerator::Sete::Sete(const std::string &dst) : dst(dst) {}

void IRGenerator::Sete::log() const {
    std::cout << "sete: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "')" << '\n';
}

IRGenerator::Setne::Setne() {}

IRGenerator::Setne::Setne(const std::string &dst) : dst(dst) {}

void IRGenerator::Setne::log() const {
    std::cout << "setne: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "')" << '\n';
}

IRGenerator::Setg::Setg() {}

IRGenerator::Setg::Setg(const std::string &dst) : dst(dst) {}

void IRGenerator::Setg::log() const {
    std::cout << "setg: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "')" << '\n';
}

IRGenerator::Setge::Setge() {}

IRGenerator::Setge::Setge(const std::string &dst) : dst(dst) {}

void IRGenerator::Setge::log() const {
    std::cout << "setge: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "')" << '\n';
}

IRGenerator::Setl::Setl() {}

IRGenerator::Setl::Setl(const std::string &dst) : dst(dst) {}

void IRGenerator::Setl::log() const {
    std::cout << "setl: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "')" << '\n';
}

IRGenerator::Setle::Setle() {}

IRGenerator::Setle::Setle(const std::string &dst) : dst(dst) {}

void IRGenerator::Setle::log() const {
    std::cout << "setle: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "')" << '\n';
}

IRGenerator::Cmove::Cmove() {}

IRGenerator::Cmove::Cmove(const std::string &dst, const std::string &src) : dst(dst), src(src) {}

void IRGenerator::Cmove::log() const {
    std::cout << "cmove: (";
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

IRGenerator::Label::Label(const std::string &id) : id(id) {}

void IRGenerator::Label::log() const {
    std::cout << "label: (";
    std::cout << "id: '";
    std::cout << id;
    std::cout << "')" << '\n';
}

IRGenerator::Jmp::Jmp(const std::string &dst) : dst(dst) {}

void IRGenerator::Jmp::log() const {
    std::cout << "jmp: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "')" << '\n';
}

IRGenerator::Je::Je(const std::string &dst) : dst(dst) {}

void IRGenerator::Je::log() const {
    std::cout << "je: (";
    std::cout << "dst: '";
    std::cout << dst;
    std::cout << "')" << '\n';
}

IRGenerator::Jne::Jne(const std::string &dst) : dst(dst) {}

void IRGenerator::Jne::log() const {
    std::cout << "jne: (";
    std::cout << "dst: '";
    std::cout << dst;
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

IRGenerator::ClassInfo::ClassInfo(const std::string &id) : id(id) {}