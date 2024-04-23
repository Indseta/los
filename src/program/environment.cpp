#include <program/environment.h>

void Environment::run(const std::string &fp) {
    Source source(fp);

    if (!source.get_success()) {
        std::cout << "Exiting due to read error." << '\n';
        return;
    }

    Lexer lexer(source);
    // for (const auto &t : lexer.get()) {
    //     t.log();
    //     std::cout << '\n';
    // }

    if (!lexer.get_success()) {
        std::cout << "Exiting due to lex error." << '\n';
        return;
    }

    Parser parser(lexer);
    // for (const auto &n : parser.get()) {
    //     n->log();
    //     std::cout << '\n';
    // }

    if (!parser.get_success()) {
        std::cout << "Exiting due to parse error." << '\n';
        return;
    }

    // Interpreter interpreter(parser);

    Compiler compiler(parser, fp);
}