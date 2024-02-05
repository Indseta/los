#include <program/interpreter.h>

Interpreter::Interpreter(const Parser &parser) {
    run(parser.get());
}

void Interpreter::run(const std::vector<std::unique_ptr<Parser::Node>> &ast) {
    for (const auto& node : ast) {
        if (const auto* exprTree = dynamic_cast<const Parser::Node*>(node.get())) {
            if (const auto* varAssign = dynamic_cast<const Parser::VariableAssignment*>(exprTree)) {
                int value = evaluate(varAssign->expr.get());
                memory[varAssign->identifier] = value;
            } else if (const auto* logExpr = dynamic_cast<const Parser::ConsoleLog*>(exprTree)) {
                int value = evaluate(logExpr->expr.get());
                std::cout << value << '\n';
            } else {
                std::cout << "Unsupported stack node type encountered\n";
            }
        } else {
            std::cout << "Unsupported AST node type encountered\n";
        }
    }
}

int Interpreter::evaluate(const Parser::Node* node) {
    if (const auto* num = dynamic_cast<const Parser::NumberLiteral*>(node)) {
        return std::stoi(num->value);
    } else if (const auto* call = dynamic_cast<const Parser::VariableCall*>(node)) {
        auto it = memory.find(call->value);
        if (it != memory.end()) {
            return it->second;
        }
        throw std::runtime_error("Undefined variable: " + call->value);
    } else if (const auto* binExp = dynamic_cast<const Parser::BinaryOperation*>(node)) {
        int left = evaluate(binExp->left.get());
        int right = evaluate(binExp->right.get());
        if (binExp->op == "+") {
            return left + right;
        } else if (binExp->op == "-") {
            return left - right;
        } else if (binExp->op == "*") {
            return left * right;
        } else if (binExp->op == "/") {
            if (right == 0) throw std::runtime_error("Division by zero error");
            return left / right;
        } else {
            throw std::runtime_error("Unsupported operator: " + binExp->op);
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