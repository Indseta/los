#include <program/interpreter.h>

Interpreter::Interpreter(const Parser &parser) {
    interpret(parser.get());
}

void Interpreter::interpret(const std::vector<std::unique_ptr<Parser::Node>> &ast) {
    for (const auto &tree : ast) {
        if (const auto *expr = dynamic_cast<const Parser::Node*>(tree.get())) {
            if (const auto *node = dynamic_cast<const Parser::VariableDeclaration*>(expr)) {
                const auto res = evaluate(node->expr.get());
                memory[node->identifier] = res;
            } else if (const auto *node = dynamic_cast<const Parser::LogStatement*>(expr)) {
                const auto res = evaluate(node->expr.get());
                std::cout << res.value << '\n';
            } else {
                std::cout << "Unsupported AST node type encountered" << '\n';
            }
        } else {
            std::cout << "Unsupported AST node type encountered" << '\n';
        }
    }
}

Interpreter::Res Interpreter::evaluate(const Parser::Node *node) {
    if (const auto *expr = dynamic_cast<const Parser::NumberLiteral*>(node)) {
        Res res(expr->value, "int");
        return res;
    } else if (const auto *expr = dynamic_cast<const Parser::VariableCall*>(node)) {
        auto r = memory.find(expr->value);
        if (r != memory.end()) {
            return r->second;
        }
        throw std::runtime_error("Undefined variable: " + expr->value);
    } else if (const auto* expr = dynamic_cast<const Parser::StringLiteral*>(node)) {
        Res res(expr->value, "string");
        return res;
    } else if (const auto *expr = dynamic_cast<const Parser::BinaryOperation*>(node)) {
        Res left = evaluate(expr->left.get());
        Res right = evaluate(expr->right.get());
        if (expr->op == "+") {
            Res res(add(left.value, right.value), "int");
            return res;
        } else if (expr->op == "-") {
            Res res(sub(left.value, right.value), "int");
            return res;
        } else if (expr->op == "*") {
            Res res(mul(left.value, right.value), "int");
            return res;
        } else if (expr->op == "/") {
            if (right.value == "0") throw std::runtime_error("Division by zero error");
            Res res(div(left.value, right.value), "int");
            return res;
        } else {
            throw std::runtime_error("Unsupported operator: " + expr->op);
        }
    } else {
        throw std::runtime_error("Unsupported AST node type");
    }
}

const std::string Interpreter::add(const std::string &a, const std::string &b) const {
    return std::to_string(std::stoi(a) + std::stoi(a));
}
const std::string Interpreter::sub(const std::string &a, const std::string &b) const {
    return std::to_string(std::stoi(a) - std::stoi(a));
}
const std::string Interpreter::mul(const std::string &a, const std::string &b) const {
    return std::to_string(std::stoi(a) * std::stoi(a));
}
const std::string Interpreter::div(const std::string &a, const std::string &b) const {
    return std::to_string(std::stoi(a) / std::stoi(a));
}

void Interpreter::log() const {
    std::cout << "Variables:\n";
    for (const auto &var : memory) {
        std::cout << var.first << " = " << var.second.value << "\n";
    }
}