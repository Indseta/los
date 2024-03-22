#include <program/interpreter.h>

Interpreter::Interpreter(const Parser &parser) {
    interpret(parser.get());
}

void Interpreter::interpret(const std::vector<std::unique_ptr<Parser::Node>> &ast) {
    for (const auto &t : ast) {
        interpret_node(t.get());
    }
}

void Interpreter::interpret_node(const Parser::Node *expr) {
    if (const auto *node = dynamic_cast<const Parser::VariableDeclaration*>(expr)) {
        auto it = heap.find(node->identifier);
        if (it == heap.end()) {
            Value val = evaluate_node(node->expr.get());
            heap[node->identifier] = val;
        } else {
            throw std::runtime_error("Variable already defined: " + node->identifier);
        }
    } else if (const auto *node = dynamic_cast<const Parser::VariableAssignment*>(expr)) {
        auto it = heap.find(node->identifier);
        if (it != heap.end()) {
            Value val = evaluate_node(node->expr.get());
            heap[node->identifier] = val;
        } else {
            throw std::runtime_error("Variable not defined: " + node->identifier);
        }
    } else if (const auto *node = dynamic_cast<const Parser::FunctionDeclaration*>(expr)) {
        std::cout << "Not implemented: Function declaration" << '\n';
    } else if (const auto *node = dynamic_cast<const Parser::ScopeDeclaration*>(expr)) {
        for (const auto &n : node->ast) {
            interpret_node(n.get());
        }
    } else if (const auto *node = dynamic_cast<const Parser::ConditionalStatement*>(expr)) {
        Value condition = evaluate_node(node->condition.get());
        if (std::holds_alternative<bool>(condition) && std::get<bool>(condition)) {
            interpret_node(node->pass_statement.get());
        } else {
            interpret_node(node->fail_statement.get());
        }
    } else if (const auto *node = dynamic_cast<const Parser::WhileLoopStatement*>(expr)) {
        while (std::holds_alternative<bool>(evaluate_node(node->condition.get())) && std::get<bool>(evaluate_node(node->condition.get()))) {
            interpret_node(node->statement.get());
        }
    } else if (const auto *node = dynamic_cast<const Parser::FunctionCall*>(expr)) {
        evaluate_function_call(node);
    } else if (const auto *node = dynamic_cast<const Parser::EmptyStatement*>(expr)) {
    } else {
        throw std::runtime_error("Unsupported node type encountered");
    }
}

Interpreter::Value Interpreter::evaluate_node(const Parser::Node *node) {
    if (const auto *n = dynamic_cast<const Parser::IntegerLiteral*>(node)) {
        return n->value;
    } else if (const auto *n = dynamic_cast<const Parser::FloatLiteral*>(node)) {
        return n->value;
    } else if (const auto *n = dynamic_cast<const Parser::BooleanLiteral*>(node)) {
        return n->value;
    } else if (const auto *n = dynamic_cast<const Parser::StringLiteral*>(node)) {
        return n->value;
    } else if (const auto *n = dynamic_cast<const Parser::VariableCall*>(node)) {
        auto it = heap.find(n->identifier);
        if (it != heap.end()) {
            return it->second;
        } else {
            throw std::runtime_error("Variable not defined: " + n->identifier);
        }
    } else if (const auto *n = dynamic_cast<const Parser::FunctionCall*>(node)) {
        return evaluate_function_call(n);
    } else if (const auto *n = dynamic_cast<const Parser::UnaryOperation*>(node)) {
        return evaluate_unary_operation(n);
    } else if (const auto *n = dynamic_cast<const Parser::BinaryOperation*>(node)) {
        return evaluate_binary_operation(n);
    }

    throw std::runtime_error("Unsupported node encountered in evaluation");
}

Interpreter::Value Interpreter::evaluate_function_call(const Parser::FunctionCall *expr) {
    if (expr->identifier == "println") {
        if (expr->args.size() == 1) {
            const auto &a = expr->args[0];
            Value val = evaluate_node(a.get());
            std::visit([](auto &&a) {
                std::cout << a << '\n';
            }, val);
            return 0;
        }
    }

    throw std::runtime_error("Function " + expr->identifier + " with " + std::to_string(expr->args.size()) + " args not defined");
}

Interpreter::Value Interpreter::evaluate_unary_operation(const Parser::UnaryOperation *expr) {
    Value operand = evaluate_node(expr->value.get());
    const std::string &op = expr->op;

    if (op == "-") {
        if (std::holds_alternative<int>(operand)) {
            return -std::get<int>(operand);
        } else if (std::holds_alternative<float>(operand)) {
            return -std::get<float>(operand);
        } else {
            throw std::runtime_error("Unsupported type for unary negation");
        }
    } else if (op == "!") {
        if (std::holds_alternative<bool>(operand)) {
            return !std::get<bool>(operand);
        } else {
            throw std::runtime_error("Unsupported type for logical NOT operator");
        }
    }

    throw std::runtime_error("Unsupported unary operator: " + op);
}

Interpreter::Value Interpreter::evaluate_binary_operation(const Parser::BinaryOperation *expr) {
    Value left = evaluate_node(expr->left.get());
    Value right = evaluate_node(expr->right.get());
    const std::string& op = expr->op;

    if (op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=") {
        return perform_comparison_operation(left, right, op);
    } else {
        return perform_arithmetic_operation(left, right, op);
    }
}

Interpreter::Value Interpreter::perform_comparison_operation(const Interpreter::Value &left, const Interpreter::Value &right, const std::string &op) {
    auto compare = [&](auto a, auto b) -> bool {
        if (op == "==") return a == b;
        if (op == "!=") return a != b;
        if (op == "<") return a < b;
        if (op == "<=") return a <= b;
        if (op == ">") return a > b;
        if (op == ">=") return a >= b;
        throw std::runtime_error("Unsupported comparison operator: " + op);
    };

    if (std::holds_alternative<float>(left) || std::holds_alternative<float>(right)) {
        float leftVal = std::holds_alternative<int>(left) ? std::get<int>(left) : std::get<float>(left);
        float rightVal = std::holds_alternative<int>(right) ? std::get<int>(right) : std::get<float>(right);
        return compare(leftVal, rightVal);
    } else if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
        return compare(std::get<int>(left), std::get<int>(right));
    }

    throw std::runtime_error("Attempt to compare non-numeric types.");
}

Interpreter::Value Interpreter::perform_arithmetic_operation(const Interpreter::Value &left, const Interpreter::Value &right, const std::string &op) {
    auto arithmetic_op = [&](auto a, auto b) -> Interpreter::Value {
        if (op == "+") return a + b;
        if (op == "-") return a - b;
        if (op == "*") return a * b;
        if (op == "/") return static_cast<float>(a) / b;
        if (op == "%") return static_cast<float>(std::fmod(a, b));
        throw std::runtime_error("Unsupported operator: " + op);
    };

    if (std::holds_alternative<float>(left) || std::holds_alternative<float>(right)) {
        float leftVal = std::holds_alternative<int>(left) ? std::get<int>(left) : std::get<float>(left);
        float rightVal = std::holds_alternative<int>(right) ? std::get<int>(right) : std::get<float>(right);
        return arithmetic_op(leftVal, rightVal);
    } else if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
        return arithmetic_op(std::get<int>(left), std::get<int>(right));
    }

    throw std::runtime_error("Attempt to perform arithmetic on non-numeric types.");
}

float Interpreter::modulo(const float &a, const float &b) {
    if (b < 0) return modulo(-a, -b);
    float res = std::fmod(a, b);
    return res >= 0 ? res : res + b;
}