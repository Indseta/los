#include <program/environment.h>

void Environment::run(const std::string &fp) {
    Source source(fp);

    if (source.get_success()) {
        Lexer lexer(source);
        // for (const auto &t : lexer.get()) {
        //     t.log();
        //     std::cout << '\n';
        // }

        Parser parser(lexer);
        // for (const auto &n : parser.get()) {
        //     n->log();
        //     std::cout << '\n';
        // }

        // Interpreter interpreter(parser);

        Compiler compiler(parser, fp);
    }
}