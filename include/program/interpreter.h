#pragma once

#include <program/parser.h>

#include <program/parser.h>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

class Interpreter {
public:
    struct Type {
        virtual ~Type() = default;
        virtual std::unique_ptr<Type> clone() const = 0;
        virtual void log() const = 0;
    };

    struct Int : public Type {
        Int(const int &value) : value(value) {}
        Int() : value(0) {}
        std::unique_ptr<Type> clone() const override {
            return std::make_unique<Int>(*this);
        }
        void log() const override {
            std::cout << value << '\n';
        }
        int value;
    };

    struct Float : public Type {
        Float(const float &value) : value(value) {}
        Float() : value(0) {}
        std::unique_ptr<Type> clone() const override {
            return std::make_unique<Float>(*this);
        }
        void log() const override {
            std::cout << value << '\n';
        }
        float value;
    };

    struct Bool : public Type {
        Bool(const bool &value) : value(value) {}
        Bool() : value(false) {}
        std::unique_ptr<Type> clone() const override {
            return std::make_unique<Bool>(*this);
        }
        void log() const override {
            std::cout << value << '\n';
        }
        bool value;
    };

    struct String : public Type {
        String(const std::string &value) : value(value) {}
        String() : value("") {}
        std::unique_ptr<Type> clone() const override {
            return std::make_unique<String>(*this);
        }
        void log() const override {
            std::cout << value << '\n';
        }
        std::string value;
    };

    Interpreter(const Parser &parser);
    void interpret(const std::vector<std::unique_ptr<Parser::Node>> &ast);

private:
    std::unique_ptr<Type> evaluate_node(const Parser::Node *node);

    std::unordered_map<std::string, std::unique_ptr<Type>> heap;
    std::unordered_map<std::string, std::vector<std::unique_ptr<Parser::Node>>> functions;
};