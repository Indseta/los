#include <program/environment.h>

void Environment::run(const std::string &fp) {
    Source main(fp);
    Lexer lexer(main);
    Parser parser(lexer);

    // for (const auto &t : lexer.get()) t.log();
    // for (const auto &n : parser.get()) n->log();

    Interpreter interpreter(parser);
}