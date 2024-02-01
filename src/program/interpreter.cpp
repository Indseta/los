#include <program/interpreter.h>

int Interpreter::evaluate(const Parser::ASTNode* node) {
	if (const auto* num = dynamic_cast<const Parser::NumberLiteral*>(node)) {
		return std::stoi(num->value);
	} else if (const auto* binExp = dynamic_cast<const Parser::BinaryExpression*>(node)) {
		int left = evaluate(binExp->left.get());
		int right = evaluate(binExp->right.get());
		if (binExp->op == "+") {
			return left + right;
		}
		// Add more operators as needed
		throw std::runtime_error("Unsupported operator");
	}
	// Add more node types as needed
	throw std::runtime_error("Unsupported AST node type");
}

void Interpreter::interpret(const std::vector<std::unique_ptr<Parser::ASTNode>>& ast) {
	for (const auto& node : ast) {
        if (const auto* exprTree = dynamic_cast<const Parser::ExpressionTree*>(node.get())) {
            if (const auto* varAssign = dynamic_cast<const Parser::VariableAssignment*>(exprTree->expression.get())) {
                int value = evaluate(varAssign->expression.get());
                variables[varAssign->identifier] = value;
            } else {
                std::cout << "Unsupported ExpressionTree node type encountered\n";
            }
        } else {
            std::cout << "Unsupported AST node type encountered\n";
        }
	}
}

void Interpreter::debug_print() const {
	std::cout << "Variables:\n";
	for (const auto& var : variables) {
		std::cout << var.first << " = " << var.second << "\n";
	}
}