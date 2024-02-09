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
        Value val = evaluate_node(node->expr.get());
        heap[node->identifier] = val;
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
        if (node->identifier == "println") {
            for (const auto &a : node->args) {
                Value val = evaluate_node(a.get());
                std::visit([](auto &&a) {
                    std::cout << a << '\n';
                }, val);
            }
        }
    } else {
        throw std::runtime_error("Unsupported node type encountered.");
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
    } else if (const auto *n = dynamic_cast<const Parser::BinaryOperation*>(node)) {
        return evaluate_binary_operation(n);
    }

    throw std::runtime_error("Unsupported node encountered in evaluation.");
}

Interpreter::Value Interpreter::evaluate_binary_operation(const Parser::BinaryOperation *expr) {
    Value left = evaluate_node(expr->left.get());
    Value right = evaluate_node(expr->right.get());
    const std::string& op = expr->op;

    if (op == "==" || op == "!=") {
        bool result = custom_compare(left, right);
        if (op == "!=") result = !result;
        return result;
    } else if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        return perform_comparison_operation(left, right, op);
    } else {
        return perform_arithmetic_operation(left, right, op);
    }
}

bool Interpreter::custom_compare(const Interpreter::Value &left, const Interpreter::Value &right) {
    if (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right)) {
    }

    if (std::holds_alternative<bool>(left) || std::holds_alternative<bool>(right)) {
        auto bool_to_numeric = [](bool b) -> float { return b ? 1.0f : 0.0f; };

        float leftVal = std::holds_alternative<bool>(left) ? bool_to_numeric(std::get<bool>(left)) : 
                        std::holds_alternative<int>(left) ? static_cast<float>(std::get<int>(left)) : 
                        std::get<float>(left);

        float rightVal = std::holds_alternative<bool>(right) ? bool_to_numeric(std::get<bool>(right)) : 
                         std::holds_alternative<int>(right) ? static_cast<float>(std::get<int>(right)) : 
                         std::get<float>(right);

        return leftVal == rightVal;
    }

    if (std::holds_alternative<int>(left) && std::holds_alternative<float>(right)) {
        return static_cast<float>(std::get<int>(left)) == std::get<float>(right);
    }
    if (std::holds_alternative<float>(left) && std::holds_alternative<int>(right)) {
        return std::get<float>(left) == static_cast<float>(std::get<int>(right));
    }

    throw std::runtime_error("Unsupported types for comparison.");
}

Interpreter::Value Interpreter::perform_comparison_operation(const Interpreter::Value &left, const Interpreter::Value &right, const std::string &op) {
    auto compare = [&](auto a, auto b) -> bool {
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