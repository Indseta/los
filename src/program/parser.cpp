#include <program/parser.h>

Parser::Parser(const Lexer &lexer) : tokens(lexer.get()) {
    parse();
}

void Parser::parse() {
    std::vector<std::unique_ptr<Node>> statements;
    while (!isAtEnd()) {
        statements.push_back(std::move(statement()));
    }
    ast = std::move(statements);
}

bool Parser::isAtEnd() const {
    return current >= tokens.size();
}

const Lexer::Token& Parser::peek() const {
    return tokens[current];
}

const Lexer::Token& Parser::previous() const {
    return tokens[current - 1];
}

const Lexer::Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::match(std::initializer_list<std::string> types) {
    for (const std::string &type : types) {
        if (peek().value == type && !isAtEnd()) {
            current++;
            return true;
        }
    }
    return false;
}

const Lexer::Token& Parser::consume(const std::string& type, const std::string& message) {
    if (peek().value == type) return tokens[current++];

    error(message);
}

void Parser::error(const std::string &msg) const {
    throw std::runtime_error(msg);
}

std::unique_ptr<Parser::Node> Parser::statement() {
    if (match({"let", "var", "const"})) return variable_declaration();
    if (match({"println"})) return log_statement();
    return expression();
}

std::unique_ptr<Parser::Node> Parser::log_statement() {
    auto value = expression();
    consume(";", "Expect ';' after value.");
    return std::make_unique<LogStatement>(std::move(value));
}

std::unique_ptr<Parser::Node> Parser::variable_declaration() {
    std::string op = previous().value;
    std::string identifier;
    if (peek().category == Lexer::TokenCategory::IDENTIFIER) {
        identifier = advance().value;
    } else {
        error("Expected identifier");
    }
    consume("=", "Expected '=' after variable identifier");
    auto value = expression();
    consume(";", "Expected ';' after variable declaration");
    return std::make_unique<VariableDeclaration>(op, identifier, std::move(value));
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
    auto left = factor();

    while (match({"+", "-"})) {
        std::string op = previous().value;
        auto right = factor();
        left = std::make_unique<BinaryOperation>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<Parser::Node> Parser::factor() {
    auto left = unary();

    while (match({"*", "/"})) {
        std::string op = previous().value;
        auto right = unary();
        left = std::make_unique<BinaryOperation>(std::move(left), op, std::move(right));
    }

    return left;
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
    if (peek().category == Lexer::TokenCategory::INTEGER_LITERAL) {
        std::string value = peek().value;
        advance();
        return std::make_unique<NumberLiteral>(value);
    } else if (peek().category == Lexer::TokenCategory::IDENTIFIER) {
        std::string value = peek().value;
        advance();
        return std::make_unique<VariableCall>(value);
    } else if (peek().category == Lexer::TokenCategory::STRING_LITERAL) {
        std::string value = peek().value;
        advance();
        return std::make_unique<StringLiteral>(value);
    } else if (match({"("})) {
        auto expr = expression();
        consume(")", "Expected ')' after expression");
        return expr;
    }

    error("Unexpected token '" + peek().value + "'");
}

const std::vector<std::unique_ptr<Parser::Node>>& Parser::get() const {
    return ast;
}