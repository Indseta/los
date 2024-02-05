#include <program/environment.h>

void Environment::run(const std::string &fp) {
	Source main(fp);
	Lexer lexer(main);
	Parser parser(lexer);
	Interpreter interpreter(parser);
	interpreter.print();
}