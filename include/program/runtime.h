#pragma once

#define NDEBUG

#include <program/interpreter.h>
#include <program/lexer.h>
#include <program/parser.h>
#include <program/source.h>

#include <iostream>

class Runtime {
public:
	void run(const std::string &fp);
};