#include <program/parser.h>

Parser::Parser(const Lexer &lexer) : tokens(lexer.get()) {
    success = true;

    parse();
}

void Parser::parse() {
    std::vector<std::unique_ptr<Node>> statements;
    while (!at_end()) {
        statements.push_back(std::move(global_statement()));
    }
    ast = std::move(statements);
}

bool Parser::at_end() const {
    return current >= tokens.size();
}

const Lexer::Token& Parser::peek() const {
    return tokens[current];
}

const Lexer::Token& Parser::previous() const {
    return tokens[current - 1];
}

const Lexer::Token& Parser::next() const {
    if (!at_end()) {
        return tokens[current + 1];
    } else {
        return peek();
    }
}

const Lexer::Token& Parser::advance() {
    if (!at_end()) current++;
    return previous();
}

bool Parser::match(const std::initializer_list<std::string> &types) {
    for (const std::string &type : types) {
        if (peek().value == type) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::match_next(const std::initializer_list<std::string> &types) {
    for (const std::string &type : types) {
        if (next().value == type) {
            return true;
        }
    }
    return false;
}

bool Parser::match_key() {
    if (peek().category == Lexer::IDENTIFIER) {
        advance();
        return true;
    }

    if (peek().category == Lexer::KEYWORD) {
        for (const auto &type : {"u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64", "f8", "f16", "f32", "f64", "bool", "string", "ptr", "ref"}) {
            if (peek().value == type) {
                advance();
                return true;
            }
        }
    }

    return false;
}

const Lexer::Token& Parser::consume(const std::string& type, const std::string& message) {
    if (peek().value == type) return tokens[current++];

    error(message);
}

void Parser::error(const std::string &msg) {
    success = false;
    throw std::runtime_error(msg);
}

std::unique_ptr<Parser::Node> Parser::global_statement() {
    if (match({"void"}) || match_key()) { // Final
        if (peek().category == Lexer::IDENTIFIER) {
            if (match_next({"("})) return function_declaration();
        }
    }
    throw std::runtime_error("Unexpected global statement encountered.");
}

std::unique_ptr<Parser::Node> Parser::statement() {
    if (match({"{"})) return scope_declaration();
    if (match({"if", "else"})) return conditional_statement();
    if (match({"while"})) return while_loop_statement();

    if (peek().category == Lexer::IDENTIFIER) {
        if (match_next({"=", "+=", "-=", "*=", "/=", "%="})) return variable_assignment();
        if (match_next({"("})) return function_call();
    }

    if (match_key()) { // Final
        if (peek().category == Lexer::IDENTIFIER) {
            if (match_next({"="})) return variable_declaration();
        }
    }

    return expression();
}

std::unique_ptr<Parser::Node> Parser::variable_declaration() {
    std::string op = previous().value;
    std::string identifier;
    if (peek().category == Lexer::IDENTIFIER) {
        identifier = advance().value;
    } else {
        error("Expected identifier");
    }
    consume("=", "Expected '='");
    auto value = expression();
    consume(";", "Expected ';' after statement");
    return std::make_unique<VariableDeclaration>(op, identifier, std::move(value));
}

std::unique_ptr<Parser::Node> Parser::function_declaration() {
    auto function = std::make_unique<FunctionDeclaration>();
    if (peek().category == Lexer::IDENTIFIER) {
        function->identifier = advance().value;
    } else {
        error("Expected identifier");
    }
    consume("(", "Expected '('");
    while (peek().value != ")") {
        function->params.push_back(advance().value); // Check if keyword
        if (peek().value != ")") {
            consume(",", "Expected ','");
        }
    }
    consume(")", "Expected ')' after statement");
    function->statement = std::move(statement());
    return function;
}

std::unique_ptr<Parser::Node> Parser::variable_assignment() {
    std::string identifier = advance().value;
    std::string op;
    if (peek().category == Lexer::OPERATOR) {
        op = advance().value;
    } else {
        error("Expected operator");
    }
    auto value = expression();
    consume(";", "Expected ';' after statement");

    if (op == "=") {
        return std::make_unique<VariableAssignment>(identifier, std::move(value));
    } else {
        auto expr = std::make_unique<BinaryOperation>();
        expr->left = std::make_unique<VariableCall>(identifier);
        if (op == "+=") {
            expr->op = "+";
        } else if (op == "-=") {
            expr->op = "-";
        } else if (op == "*=") {
            expr->op = "*";
        } else if (op == "/=") {
            expr->op = "/";
        } else if (op == "%=") {
            expr->op = "%";
        }
        expr->right = std::move(value);

        return std::make_unique<VariableAssignment>(identifier, std::move(expr));
    }
}

std::unique_ptr<Parser::Node> Parser::scope_declaration() {
    auto scope = std::make_unique<ScopeDeclaration>();
    while (peek().value != "}") {
        scope->ast.push_back(std::move(statement()));
    }
    advance(); // TODO: consume
    return scope;
}

std::unique_ptr<Parser::Node> Parser::conditional_statement() {
    auto conditional = std::make_unique<ConditionalStatement>();
    consume("(", "Expected '('");
    conditional->condition = std::move(expression());
    consume(")", "Expected ')'");
    conditional->pass_statement = std::move(statement());
    if (peek().value == "else") {
        advance();
        conditional->fail_statement = std::move(statement());
    }
    return conditional;
}

std::unique_ptr<Parser::Node> Parser::while_loop_statement() {
    auto loop_statement = std::make_unique<WhileLoopStatement>();
    consume("(", "Expected '('");
    loop_statement->condition = std::move(expression());
    consume(")", "Expected ')'");
    loop_statement->statement = std::move(statement());
    return loop_statement;
}

std::unique_ptr<Parser::Node> Parser::function_call() {
    auto function = std::make_unique<FunctionCall>(advance().value);
    advance();
    while (peek().value != ")") {
        function->args.push_back(std::move(expression()));
        if (peek().value != ")") {
            consume(",", "Expected ','");
        }
    }
    advance();
    consume(";", "Expected ';' after statement");
    return function;
}

std::unique_ptr<Parser::Node> Parser::expression() {
    return equality();
}

std::unique_ptr<Parser::Node> Parser::equality() {
    auto expr = comparison();

    while (match({"==", "!="})) {
        std::string op = previous().value;
        auto right = comparison();
        expr = std::make_unique<BinaryOperation>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Parser::Node> Parser::comparison() {
    auto expr = term();

    while (match({"<", "<=", ">", ">="})) {
        std::string op = previous().value;
        auto right = term();
        expr = std::make_unique<BinaryOperation>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Parser::Node> Parser::term() {
    auto expr = factor();

    while (match({"+", "-"})) {
        std::string op = previous().value;
        auto right = factor();
        expr = std::make_unique<BinaryOperation>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Parser::Node> Parser::factor() {
    auto expr = remainder();

    while (match({"*", "/"})) {
        std::string op = previous().value;
        auto right = remainder();
        expr = std::make_unique<BinaryOperation>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Parser::Node> Parser::remainder() {
    auto expr = unary();

    while (match({"%"})) {
        std::string op = previous().value;
        auto right = unary();
        expr = std::make_unique<BinaryOperation>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Parser::Node> Parser::unary() {
    if (match({"-", "!"})) {
        std::string op = previous().value;
        auto right = unary();
        return std::make_unique<UnaryOperation>(op, std::move(right));
    }

    return primary();
}

std::unique_ptr<Parser::Node> Parser::primary() {
    if (peek().category == Lexer::INTEGER_LITERAL) {
        return std::make_unique<IntegerLiteral>(std::stoi(advance().value));
    } else if (peek().category == Lexer::FLOAT_LITERAL) {
        return std::make_unique<FloatLiteral>(std::stof(advance().value));
    } else if (peek().category == Lexer::BOOLEAN_LITERAL) {
        return std::make_unique<BooleanLiteral>(advance().value == "true");
    } else if (peek().category == Lexer::STRING_LITERAL) {
        return std::make_unique<StringLiteral>(advance().value);
    } else if (peek().category == Lexer::IDENTIFIER) {
        if (next().value == "(") {
            auto res = std::make_unique<FunctionCall>(advance().value);
            consume("(", "Expected '('");
            consume(")", "Expected ')'");
            return res;
        } else {
            return std::make_unique<VariableCall>(advance().value);
        }
    } else if (match({"("})) {
        auto expr = expression();
        consume(")", "Expected ')' after expression");
        return expr;
    }
    std::cout << tokens[current - 2].value << previous().value << peek().value << " < ";
    error("Unexpected token '" + peek().value + "'");
}

const std::vector<std::unique_ptr<Parser::Node>>& Parser::get() const {
    return ast;
}

const bool& Parser::get_success() const {
    return success;
}

void Parser::log() const {
    std::cout << " -- Parse result -- " << '\n';
    for (const auto &n : ast) {
        n->log();
        std::cout << '\n';
    }
    std::cout << '\n';
}