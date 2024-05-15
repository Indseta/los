#pragma once

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>

#include <program/parser.h>

class IRGenerator {
public:
    enum IntegralType {
        UINT,
        INT,
        FLOAT,
        BOOL,
        STRING,
        UNKNOWN,
    };

    struct TypeInfo {
        TypeInfo();
        std::string name;
        IntegralType type;
        int size;
    };

    struct StackEntry {
        StackEntry();
        TypeInfo type;
        int offset;
    };

    struct StackInfo {
        StackInfo();
        std::map<std::string, StackEntry> keys;
        int size;

        StackEntry get(const std::string &id);
        int get_bottom();
        bool exists(const std::string &id);
        void push(const std::string &id, const TypeInfo &type);
    };

    struct Statement {
        Statement();
        virtual void log() const;
    };
    struct Declaration : public Statement {
        Declaration();
        Declaration(const std::string &id, const std::string &type);
        std::string id;
        std::string type;
        virtual void log() const override;
    };
    struct Db : public Declaration {
        Db();
        Db(const std::string &id, const std::string &value, const std::string &terminator);
        std::string value;
        std::string terminator;
        virtual void log() const override;
    };
    struct Resb : public Declaration {
        Resb();
        Resb(const std::string &id, const int &fac, const std::string &type);
        int fac;
        virtual void log() const override;
    };
    struct Resw : public Declaration {
        Resw();
        Resw(const std::string &id, const int &fac, const std::string &type);
        int fac;
        virtual void log() const override;
    };
    struct Resd : public Declaration {
        Resd();
        Resd(const std::string &id, const int &fac, const std::string &type);
        int fac;
        virtual void log() const override;
    };
    struct Resq : public Declaration {
        Resq();
        Resq(const std::string &id, const int &fac, const std::string &type);
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
        StackInfo args_stack;
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
    struct Movsx : public Instruction {
        Movsx();
        Movsx(const std::string &dst, const std::string &src);
        std::string dst, src;
        void log() const override;
    };
    struct Lea : public Instruction {
        Lea();
        Lea(const std::string &dst, const std::string &src);
        std::string dst, src;
        void log() const override;
    };
    struct Neg : public Instruction {
        Neg();
        Neg(const std::string &dst);
        std::string dst;
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
    struct Cmp : public Instruction {
        Cmp();
        Cmp(const std::string &left, const std::string &right);
        std::string left, right;
        void log() const override;
    };
    struct Cmove : public Instruction {
        Cmove();
        Cmove(const std::string &dst, const std::string &src);
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

    void evaluate_wrapper_statement(const Parser::Node *statement, Entry *entry);

    void evaluate_statement(const Parser::Node *statement, Entry *entry, StackInfo &stack_info);
    void evaluate_function_call(const Parser::FunctionCall *call, Entry *entry, StackInfo &stack_info);
    void evaluate_variable_declaration(const Parser::VariableDeclaration *decl, Entry *entry, StackInfo &stack_info);
    void evaluate_variable_assignment(const Parser::VariableAssignment *assign, Entry *entry, StackInfo &stack_info);

    void evaluate_expr(const Parser::Node *expr, Entry *entry, const std::string &target, StackInfo &stack_info);
    void evaluate_unary_operation(const Parser::UnaryOperation *operation, Entry *entry, const std::string &target, StackInfo &stack_info);
    void evaluate_binary_operation(const Parser::BinaryOperation *operation, Entry *entry, const std::string &target, StackInfo &stack_info);
    void evaluate_cast_operation(const Parser::CastOperation *operation, Entry *entry, const std::string &target, StackInfo &stack_info);

    void evaluate_variable_call(const Parser::VariableCall *call, Entry *entry, const std::string &target, StackInfo &stack_info);

    void push_unique(std::unique_ptr<Declaration> decl, Segment &target);
    void add_extern(const std::string &id);

    const TypeInfo get_type_info(const std::string &name);
    const TypeInfo get_type_info(const Parser::Node *expr, Entry *entry, StackInfo &stack_info);
    const std::string get_hash(const std::string &src, const std::string &prefix = "d") const;
    const bool match_type(const std::string &id, const std::initializer_list<std::string> &types) const;

    std::string get_registry(const std::string &top_name, const int &size);
    std::string get_word(const int &size);
    IntegralType get_integral_type(const std::string &name);
    int get_data_size(const std::string &name);
    int align_by(const int &src, const int &size);

    std::unordered_map<std::string, Parser::FunctionDeclaration*> functions;
    std::vector<std::string> ext_libs;
    Segment data;
    Segment bss;
    Segment text;

    bool success;
};