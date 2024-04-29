#pragma once

#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

#include <program/parser.h>

class IRGenerator {
public:
    struct Statement {
        Statement();
        virtual void log() const;
    };
    struct Declaration : public Statement {
        Declaration();
        virtual void log() const override;
    };
    struct Db : public Declaration {
        Db();
        Db(const std::string &id, const std::string &value);
        std::string id;
        std::string value;
        virtual void log() const override;
    };
    struct Resb : public Declaration {
        Resb();
        Resb(const std::string &id, const int &fac);
        std::string id;
        int fac;
        virtual void log() const override;
    };
    struct Resw : public Declaration {
        Resw();
        Resw(const std::string &id, const int &fac);
        std::string id;
        int fac;
        virtual void log() const override;
    };
    struct Resd : public Declaration {
        Resd();
        Resd(const std::string &id, const int &fac);
        std::string id;
        int fac;
        virtual void log() const override;
    };
    struct Resq : public Declaration {
        Resq();
        Resq(const std::string &id, const int &fac);
        std::string id;
        int fac;
        virtual void log() const override;
    };
    struct Segment : public Statement {
        Segment();
        std::vector<std::unique_ptr<Declaration>> declarations;
        virtual void log() const override;
    };
    struct Instruction : public Statement {
        Instruction();
        virtual void log() const override;
    };
    struct Entry : public Declaration {
        Entry();
        Entry(const std::string &id);
        std::string id;
        std::vector<std::unique_ptr<Instruction>> instructions;
        virtual void log() const override;
    };
    struct Push : public Instruction {
        Push();
        Push(const std::string &src);
        std::string src;
        void log() const override;
    };
    struct Mov : public Instruction {
        Mov();
        Mov(const std::string &dst, const std::string &src);
        std::string dst, src;
        void log() const override;
    };
    struct Lea : public Instruction {
        Lea();
        Lea(const std::string &dst, const std::string &src);
        std::string dst, src;
        void log() const override;
    };
    struct Imul : public Instruction {
        Imul();
        Imul(const std::string &dst, const std::string &src);
        std::string dst, src;
        void log() const override;
    };
    struct Idiv : public Instruction {
        Idiv();
        Idiv(const std::string &src);
        std::string src;
        void log() const override;
    };
    struct Add : public Instruction {
        Add();
        Add(const std::string &dst, const std::string &src);
        std::string dst, src;
        void log() const override;
    };
    struct Sub : public Instruction {
        Sub();
        Sub(const std::string &dst, const std::string &src);
        std::string dst, src;
        void log() const override;
    };
    struct Xor : public Instruction {
        Xor();
        Xor(const std::string &dst, const std::string &src);
        std::string dst, src;
        void log() const override;
    };
    struct Leave : public Instruction {
        Leave();
        void log() const override;
    };
    struct Ret : public Instruction {
        Ret();
        void log() const override;
    };
    struct Call : public Instruction {
        Call();
        Call(const std::string &id);
        std::string id;
        void log() const override;
    };

    IRGenerator(const Parser &parser);

    const std::vector<std::string>& get_ext_libs() const;
    const Segment& get_data() const;
    const Segment& get_bss() const;
    const Segment& get_text() const;

    const bool& get_success() const;
    void log() const;

private:
    void generate_ir(const std::vector<std::unique_ptr<Parser::Node>> &ast);

    void evaluate_global_statement(const Parser::Node *statement);
    void evaluate_function_declaration(const Parser::FunctionDeclaration *decl);

    void evaluate_statement(const Parser::Node *statement, Entry *entry);
    void evaluate_function_call(const Parser::FunctionCall *call, Entry *entry);
    void evaluate_variable_declaration(const Parser::VariableDeclaration *decl, Entry *entry);

    void evaluate_expr(const Parser::Node *expr, Entry *entry, const std::string &target);
    void evaluate_unary_operation(const Parser::UnaryOperation *operation, Entry *entry, const std::string &target);
    void evaluate_binary_operation(const Parser::BinaryOperation *operation, Entry *entry, const std::string &target);

    void add_extern(const std::string &id);
    std::string get_hash(const size_t &size);

    std::vector<std::string> ext_libs;
    Segment data;
    Segment bss;
    Segment text;

    bool success;
};