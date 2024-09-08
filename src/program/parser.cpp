#include <program/parser.h>

Parser::Parser(const Lexer &lexer) : tokens(lexer.get()) {
    success = true;
    mod_prefix = "";

    parse();
}

void Parser::parse() {
    std::vector<std::unique_ptr<Node>> statements;
    while (match({"use"})) {
        rewind();
        statements.push_back(std::move(flag_statement()));
    }

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

const Lexer::Token& Parser::rewind() {
    if (!at_end()) current--;
    return next();
}

const Lexer::Token& Parser::advance() {
    if (!at_end()) current++;
    return previous();
}

bool Parser::match(const std::initializer_list<std::string> &values) {
    for (const std::string &type : values) {
        if (peek().value == type) {
            advance();
            return true;
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
    throw std::runtime_error("[Line " + std::to_string(peek().line) + "] " + msg);
}

std::unique_ptr<Parser::Node> Parser::flag_statement() {
    if (match({"use"})) return extern_declaration();
    throw std::runtime_error("Unexpected flag statement encountered.");
}

std::unique_ptr<Parser::Node> Parser::global_statement() {
    if (match({"use"})) return extern_declaration();
    if (match({"module"})) return module_declaration();
    if (match({"class"})) return class_declaration();
    if (peek().category == Lexer::IDENTIFIER) {
        advance();
        if (peek().category == Lexer::IDENTIFIER) {
            advance();
            if (match({"("})) return function_declaration();
        }
    }
    throw std::runtime_error("Unexpected global statement encountered.");
}

std::unique_ptr<Parser::Node> Parser::statement() {
    if (match({"{"})) return scope_declaration();
    if (match({"if", "else"})) return conditional_statement();
    if (match({"while"})) return while_loop_statement();
    if (match({"return"})) return return_statement();

    if (peek().category == Lexer::IDENTIFIER) {
        std::string mod = "";
        advance();
        return modular_statement(mod);
    }

    if (match({";"})) return std::make_unique<EmptyStatement>();

    return expression();
}

std::unique_ptr<Parser::Node> Parser::modular_statement(std::string &mod) {
    if (match({"."})) {
        rewind();
        mod += previous().value + ".";
        advance();
        advance();
        return modular_statement(mod);
    }

    if (match({"=", "+=", "-=", "*=", "/=", "%="})) return variable_assignment(mod);
    if (match({"("})) return function_call(mod);

    if (peek().category == Lexer::IDENTIFIER) {
        advance();
        if (match({"="})) return variable_declaration(true);
        else return variable_declaration(false);
    }

    return expression();
}

std::unique_ptr<Parser::Node> Parser::variable_declaration(const bool &initialized) {
    for (int i = 0; i < (initialized ? 3 : 2); ++i) rewind();
    std::string type, identifier;
    if (peek().category == Lexer::IDENTIFIER) {
        type = advance().value;
    } else {
        error("Expected variable type, not '" + peek().value + "'");
    }
    if (peek().category == Lexer::IDENTIFIER) {
        identifier = advance().value;
    } else {
        error("Expected variable type, not '" + peek().value + "'");
    }

    if (initialized) {
        consume("=", "Expected '='");
        auto value = expression();
        consume(";", "Expected ';' after statement");
        return std::make_unique<VariableDeclaration>(type, identifier, std::move(value));
    } else {
        auto value = std::make_unique<EmptyStatement>();
        consume(";", "Expected ';' after statement");
        return std::make_unique<VariableDeclaration>(type, identifier, std::move(value));
    }
}

std::unique_ptr<Parser::Node> Parser::function_declaration() {
    auto function = std::make_unique<FunctionDeclaration>();
    for (int i = 0; i < 3; ++i) rewind();
    if (peek().category == Lexer::IDENTIFIER) {
        function->type = advance().value;
    } else {
        error("Expected function type");
    }
    if (peek().category == Lexer::IDENTIFIER) {
        function->identifier = mod_prefix + advance().value;
    } else {
        error("Expected function identifer");
    }
    consume("(", "Expected '('");
    while (peek().value != ")") {
        function->args_types.push_back(advance().value);
        function->args_ids.push_back(advance().value);
        if (peek().value != ")") {
            consume(",", "Expected ','");
        }
    }
    consume(")", "Expected ')' after statement");
    function->statement = std::move(statement());
    return function;
}

std::unique_ptr<Parser::Node> Parser::extern_declaration() {
    std::string path = "";
    while (!match({";"})) {
        path += peek().value;
        advance();
    }
    return std::make_unique<Extern>(path);
}

std::unique_ptr<Parser::Node> Parser::module_declaration() {
    auto mod = std::make_unique<Module>();
    mod->id = advance().value;
    mod_prefix += mod->id + ".";
    if (match({"{"})) {
        auto scope = std::make_unique<ScopeDeclaration>();
        while (!match({"}"})) {
            scope->ast.push_back(std::move(global_statement()));
        }
        mod->statement = std::move(scope);
    } else {
        mod->statement = std::move(global_statement());
    }
    mod_prefix = "";
    return mod;
}

std::unique_ptr<Parser::Node> Parser::class_declaration() {
    auto decl = std::make_unique<ClassDeclaration>();
    decl->identifier = advance().value;
    if (match({"{"})) {
        auto scope = std::make_unique<ScopeDeclaration>();
        while (!match({"}"})) {
            scope->ast.push_back(std::move(class_statement()));
        }
        decl->statement = std::move(scope);
    } else {
        decl->statement = std::move(class_statement());
    }
    return decl;
}

std::unique_ptr<Parser::Node> Parser::class_statement() {
    auto member = std::make_unique<ClassMember>();

    if (match({"public", "protected", "private"})) {
        if (previous().value == "public") member->access = ClassMember::PUBLIC;
        if (previous().value == "protected") member->access = ClassMember::PROTECTED;
        if (previous().value == "private") member->access = ClassMember::PRIVATE;
    } else {
        member->access = ClassMember::PRIVATE;
    }

    if (match({"constructor"})) member->statement = constructor_declaration();
    else if (match({"destructor"})) member->statement = destructor_declaration();
    else if (peek().category == Lexer::IDENTIFIER) {
        advance();
        if (peek().category == Lexer::IDENTIFIER) {
            advance();
            if (match({"("})) member->statement = function_declaration();
            else if (match({"="})) member->statement = variable_declaration(true);
            else member->statement = variable_declaration(false);
        }
    } else {
        throw std::runtime_error("Unexpected class member statement encountered: " + peek().value);
    }

    return member;
}

std::unique_ptr<Parser::Node> Parser::constructor_declaration() {
    auto function = std::make_unique<FunctionDeclaration>();
    rewind();
    if (peek().category == Lexer::KEYWORD) {
        function->type = peek().value;
        function->identifier = advance().value;
    } else {
        error("Expected function type");
    }
    consume("(", "Expected '('");
    while (peek().value != ")") {
        function->args_types.push_back(advance().value);
        function->args_ids.push_back(advance().value);
        if (peek().value != ")") {
            consume(",", "Expected ','");
        }
    }
    consume(")", "Expected ')' after statement");
    function->statement = std::move(statement());
    return function;
}

std::unique_ptr<Parser::Node> Parser::destructor_declaration() {
    auto function = std::make_unique<FunctionDeclaration>();
    rewind();
    if (peek().category == Lexer::KEYWORD) {
        function->type = peek().value;
        function->identifier = advance().value;
    } else {
        error("Expected function type");
    }
    consume("(", "Expected '('");
    while (peek().value != ")") {
        function->args_types.push_back(advance().value);
        function->args_ids.push_back(advance().value);
        if (peek().value != ")") {
            consume(",", "Expected ','");
        }
    }
    consume(")", "Expected ')' after statement");
    function->statement = std::move(statement());
    return function;
}

std::unique_ptr<Parser::Node> Parser::variable_assignment(const std::string &mod) {
    for (int i = 0; i < 2; ++i) rewind();
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
    while (!match({"}"})) {
        scope->ast.push_back(std::move(statement()));
    }
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

std::unique_ptr<Parser::Node> Parser::return_statement() {
    auto return_statement = std::make_unique<ReturnStatement>();
    if (peek().value == ";") {
        return_statement->expr = std::make_unique<EmptyStatement>();
    } else {
        return_statement->expr = std::move(expression());
    }
    consume(";", "Expected ';' after statement");
    return return_statement;
}


std::unique_ptr<Parser::Node> Parser::function_call(const std::string &mod) {
    for (int i = 0; i < 2; ++i) rewind();
    auto function = std::make_unique<FunctionCall>(mod + advance().value);
    consume("(", "Expected '('");
    while (!match({")"})) {
        function->args.push_back(std::move(expression()));
        if (peek().value != ")") {
            consume(",", "Expected ','");
        }
    }
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
    auto expr = cast();

    while (match({"<", "<=", ">", ">="})) {
        std::string op = previous().value;
        auto right = cast();
        expr = std::make_unique<BinaryOperation>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Parser::Node> Parser::cast() {
    auto expr = term();

    while (match({"as"})) {
        expr = std::make_unique<CastOperation>(std::move(expr), advance().value);
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
        return std::make_unique<IntegerLiteral>(advance().value);
    } else if (peek().category == Lexer::FLOAT_LITERAL) {
        return std::make_unique<FloatLiteral>(advance().value);
    } else if (peek().category == Lexer::BOOLEAN_LITERAL) {
        return std::make_unique<BooleanLiteral>(advance().value == "true");
    } else if (peek().category == Lexer::STRING_LITERAL) {
        return std::make_unique<StringLiteral>(advance().value);
    } else if (peek().category == Lexer::IDENTIFIER) {
        if (next().value == "(") {
            auto function = std::make_unique<FunctionCall>(advance().value);
            advance();
            while (peek().value != ")") {
                function->args.push_back(std::move(expression()));
                if (peek().value != ")") {
                    consume(",", "Expected ','");
                }
            }
            consume(")", "Expected ')'");
            return function;
        } else {
            return std::make_unique<VariableCall>(advance().value);
        }
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