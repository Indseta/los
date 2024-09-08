#pragma once

#include <program/env.h>
#include <program/lexer.h>

#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <set>
#include <vector>

class Parser {
public:
    struct Node {
        virtual ~Node() = default;
        virtual void log() const {}
    };

    struct BinaryOperation : public Node {
        BinaryOperation() {}
        BinaryOperation(std::unique_ptr<Node> left, const std::string &op, std::unique_ptr<Node> right) : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
        void log() const override {
            std::cout << "BinaryOperation: (left: (";
            left->log();
            std::cout << "), op: '" << op << "', right: (";
            right->log();
            std::cout << "))";
        }
        std::unique_ptr<Node> left;
        std::string op;
        std::unique_ptr<Node> right;
    };

    struct CastOperation : public Node {
        CastOperation() {}
        CastOperation(std::unique_ptr<Node> left, const std::string &right) : left(std::move(left)), right(right) {}
        void log() const override {
            std::cout << "CastOperation: (left: (";
            left->log();
            std::cout << "), right: '" << right << "')";
        }
        std::unique_ptr<Node> left;
        std::string right;
    };

    struct UnaryOperation : public Node {
        UnaryOperation(const std::string &op, std::unique_ptr<Node> value) : op(std::move(op)), value(std::move(value)) {}
        void log() const override {
            std::cout << "UnaryOperation: (op: '" << op << "', value: (";
            value->log();
            std::cout << "))";
        }
        std::string op;
        std::unique_ptr<Node> value;
    };

    struct IntegerLiteral : public Node {
        IntegerLiteral(const std::string &value) : value(std::move(value)) {}
        void log() const override {
            std::cout << "IntegerLiteral: '" << value << "'";
        }
        std::string value;
    };

    struct FloatLiteral : public Node {
        FloatLiteral(const std::string &value) : value(std::move(value)) {}
        void log() const override {
            std::cout << "FloatLiteral: '" << value << "'";
        }
        std::string value;
    };

    struct BooleanLiteral : public Node {
        BooleanLiteral(const bool &value) : value(std::move(value)) {}
        void log() const override {
            std::cout << "BooleanLiteral: '";
            if (value) {
                std::cout << "true";
            } else {
                std::cout << "false";
            }
            std::cout << "'";
        }
        bool value;
    };

    struct StringLiteral : public Node {
        StringLiteral(const std::string &value) : value(std::move(value)) {}
        void log() const override {
            std::cout << "StringLiteral: '" << value << "'";
        }
        std::string value;
    };

    struct VariableDeclaration : public Node {
        VariableDeclaration(const std::string &type, const std::string &identifier, std::unique_ptr<Node> expr) : type(std::move(type)), identifier(std::move(identifier)), expr(std::move(expr)) {}
        void log() const override {
            std::cout << "VariableDeclaration: (type: '" << type << "', identifier: '" << identifier << "', expr: (";
            expr->log();
            std::cout << "))";
        }
        std::string type;
        std::string identifier;
        std::unique_ptr<Node> expr;
    };

    struct VariableAssignment : public Node {
        VariableAssignment(const std::string &identifier, std::unique_ptr<Node> expr) : identifier(std::move(identifier)), expr(std::move(expr)) {}
        void log() const override {
            std::cout << "VariableAssignment: (identifier: '" << identifier << "', expr: (";
            expr->log();
            std::cout << "))";
        }
        std::string identifier;
        std::unique_ptr<Node> expr;
    };

    struct VariableCall : public Node {
        VariableCall(const std::string &identifier) : identifier(std::move(identifier)) {}
        void log() const override {
            std::cout << "VariableCall: '" << identifier << "'";
        }
        std::string identifier;
    };

    struct FunctionDeclaration : public Node {
        FunctionDeclaration() : statement(std::make_unique<EmptyStatement>()) {}
        void log() const override {
            std::cout << "FunctionDeclaration: (identifier: '" << identifier << "', args: (";
            for (size_t i = 0; i < args_ids.size(); ++i) {
                const bool last = i == args_ids.size() - 1;
                std::cout << '(' << args_types[i] << ", " << args_ids[i] << ')';
                if (!last) {
                    std::cout << ", ";
                }
            }
            std::cout << "), statement: (";
            statement->log();
            std::cout << "))";
        }
        std::string type;
        std::string identifier;
        std::vector<std::string> args_ids;
        std::vector<std::string> args_types;
        std::unique_ptr<Node> statement;
    };

    struct ReturnStatement : public Node {
        ReturnStatement() : expr(nullptr) {}
        ReturnStatement(std::unique_ptr<Node> expr) : expr(std::move(expr)) {}
        void log() const override {
            std::cout << "ReturnStatement: (expr: (";
            expr->log();
            std::cout << "))";
        }
        std::unique_ptr<Node> expr;
    };

    struct FunctionCall : public Node {
        FunctionCall(const std::string &identifier) : identifier(std::move(identifier)) {}
        void log() const override {
            std::cout << "FunctionCall: (identifier: '" << identifier << "', args: (";
            for (size_t i = 0; i < args.size(); ++i) {
                const bool last = i == args.size() - 1;
                args[i]->log();
                if (!last) {
                    std::cout << ", ";
                }
            }
            std::cout << "))";
        }
        std::string identifier;
        std::vector<std::unique_ptr<Node>> args;
    };

    struct ConditionalStatement : public Node {
        ConditionalStatement() : pass_statement(std::make_unique<EmptyStatement>()), fail_statement(std::make_unique<EmptyStatement>()) {}
        void log() const override {
            std::cout << "ConditionalStatement: (condition: (";
            condition->log();
            std::cout << "), pass_statement: (";
            pass_statement->log();
            std::cout << "), fail_statement: (";
            fail_statement->log();
            std::cout << "))";
        }
        std::unique_ptr<Node> condition;
        std::unique_ptr<Node> pass_statement;
        std::unique_ptr<Node> fail_statement;
    };

    struct ScopeDeclaration : public Node {
        ScopeDeclaration() {}
        void log() const override {
            std::cout << "ScopeDeclaration: (ast: {" << '\n';
            for (const auto &n : ast) {
                std::cout << '\t';
                n->log();
                std::cout << '\n';
            }
            std::cout << "})";
        }
        std::vector<std::unique_ptr<Node>> ast;
    };

    struct EmptyStatement : public Node {
        EmptyStatement() {}
        void log() const override {
            std::cout << "EmptyStatement";
        }
    };

    struct WhileLoopStatement : public Node {
        WhileLoopStatement() : statement(std::make_unique<EmptyStatement>()) {}
        void log() const override {
            std::cout << "WhileLoopStatement: (condition: (";
            condition->log();
            std::cout << "), statement: (";
            statement->log();
            std::cout << "))";
        }
        std::unique_ptr<Node> condition;
        std::unique_ptr<Node> statement;
    };

    struct ClassMember : public Node {
        ClassMember() : statement(std::make_unique<EmptyStatement>()) {}
        std::unique_ptr<Node> statement;

        enum {
            PUBLIC,
            PROTECTED,
            PRIVATE,
        } access;

        void log() const override {
            std::string access_str;
            if (access == PUBLIC) access_str = "public";
            else if (access == PROTECTED) access_str = "protected";
            else if (access == PRIVATE) access_str = "private";

            std::cout << "ClassMember: (access: " << access_str << ", statement: (";
            statement->log();
            std::cout << ")";
        }
    };

    struct ClassDeclaration : public Node {
        ClassDeclaration() {}
        void log() const override {
            std::cout << "ClassDeclaration: (identifier: '" << identifier << "', statement:" << '\n';
            statement->log();
            std::cout << ")";
        }

        std::string identifier;
        std::unique_ptr<Node> statement;
    };

    struct Extern : public Node {
        Extern(const std::string &id) : id(id) {}
        void log() const override {
            std::cout << "Extern: (id: '" << id << "')";
        }
        std::string id;
    };

    struct Module : public Node {
        Module() : statement(std::make_unique<EmptyStatement>()) {}
        void log() const override {
            std::cout << "Module: (id: '" << id << "', statement; (";
            statement->log();
            std::cout << "))";
        }
        std::string id;
        std::unique_ptr<Node> statement;
    };

    Parser(const Lexer &lexer);
    const std::vector<std::unique_ptr<Node>>& get() const;

    const bool& get_success() const;

    void log() const;

private:
    std::vector<std::unique_ptr<Node>> ast;
    const std::vector<Lexer::Token> &tokens;
    size_t current = 0;

    void parse();
    bool at_end() const;
    const Lexer::Token& peek() const;
    const Lexer::Token& previous() const;
    const Lexer::Token& next() const;
    const Lexer::Token& rewind();
    const Lexer::Token& advance();
    const Lexer::Token& consume(const std::string& type, const std::string& message);

    bool match(const std::initializer_list<std::string> &values);

    std::unique_ptr<Node> flag_statement();
    std::unique_ptr<Node> global_statement();
    std::unique_ptr<Node> function_declaration();

    std::unique_ptr<Node> extern_declaration();
    std::unique_ptr<Node> module_declaration();

    std::unique_ptr<Node> class_declaration();
    std::unique_ptr<Node> class_statement();
    std::unique_ptr<Node> constructor_declaration();
    std::unique_ptr<Node> destructor_declaration();

    std::unique_ptr<Node> statement();
    std::unique_ptr<Node> modular_statement(std::string &mod);
    std::unique_ptr<Node> conditional_statement();
    std::unique_ptr<Node> scope_declaration();
    std::unique_ptr<Node> variable_declaration(const bool &initialized);
    std::unique_ptr<Node> variable_assignment(const std::string &mod);
    std::unique_ptr<Node> while_loop_statement();
    std::unique_ptr<Node> return_statement();
    std::unique_ptr<Node> function_call(const std::string &mod);

    std::unique_ptr<Node> expression();
    std::unique_ptr<Node> equality();
    std::unique_ptr<Node> comparison();
    std::unique_ptr<Node> term();
    std::unique_ptr<Node> factor();
    std::unique_ptr<Node> remainder();
    std::unique_ptr<Node> cast();
    std::unique_ptr<Node> unary();
    std::unique_ptr<Node> primary();

    bool success;
    std::string mod_prefix;

    [[noreturn]] void error(const std::string &msg);
};