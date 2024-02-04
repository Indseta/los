#include <program/runtime.h>

void Runtime::run(const std::string &fp) {
	Source main(fp);
	Parser parser(fp);
	Interpreter interpreter;
	interpreter.interpret(parser.ast);
	interpreter.debug_print();
}