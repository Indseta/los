#include <program/interpreter.h>

Interpreter::Interpreter(const Parser &parser) {
    interpret(parser.get());
}

void Interpreter::interpret(const std::vector<std::unique_ptr<Parser::Node>> &ast) {
    for (const auto &tree : ast) {
        if (const auto *expr = dynamic_cast<const Parser::Node*>(tree.get())) {
            if (const auto *node = dynamic_cast<const Parser::VariableAssignment*>(expr)) {
                const auto value = evaluate(node->expr.get());
                memory[node->identifier] = value;
            } else if (const auto *node = dynamic_cast<const Parser::LogStatement*>(expr)) {
                const auto value = evaluate(node->expr.get());
                std::cout << value << '\n';
            } else {
                std::cout << "Unsupported AST node type encountered" << '\n';
            }
        } else {
            std::cout << "Unsupported AST node type encountered" << '\n';
        }
    }
}

int Interpreter::evaluate(const Parser::Node *node) {
    if (const auto* num = dynamic_cast<const Parser::NumberLiteral*>(node)) {
        return std::stoi(num->value);
    } else if (const auto* call = dynamic_cast<const Parser::VariableCall*>(node)) {
        auto it = memory.find(call->value);
        if (it != memory.end()) {
            return it->second;
        }
        throw std::runtime_error("Undefined variable: " + call->value);
    } else if (const auto *bin_exp = dynamic_cast<const Parser::BinaryOperation*>(node)) {
        int left = evaluate(bin_exp->left.get());
        int right = evaluate(bin_exp->right.get());
        if (bin_exp->op == "+") {
            return left + right;
        } else if (bin_exp->op == "-") {
            return left - right;
        } else if (bin_exp->op == "*") {
            return left * right;
        } else if (bin_exp->op == "/") {
            if (right == 0) throw std::runtime_error("Division by zero error");
            return left / right;
        } else {
            throw std::runtime_error("Unsupported operator: " + bin_exp->op);
        }
    } else {
        throw std::runtime_error("Unsupported AST node type");
    }
}

void Interpreter::log() const {
    std::cout << "Variables:\n";
    for (const auto &var : memory) {
        std::cout << var.first << " = " << var.second << "\n";
    }
}