#include <program/environment.h>
// #define NLOG

void Environment::run(const std::string &src, const std::string &out, const bool &should_run) {
#ifndef NLOG
    auto start = std::chrono::high_resolution_clock::now();
#endif

    Source source(src);
    if (!source.get_success()) {
        std::cout << "Exiting due to read error." << '\n';
        return;
    }

    Lexer lexer(source);
    if (!lexer.get_success()) {
        std::cout << "Exiting due to lex error." << '\n';
        return;
    }

#ifndef NLOG
    lexer.log();
#endif

    Parser parser(lexer);
    if (!parser.get_success()) {
        std::cout << "Exiting due to parse error." << '\n';
        return;
    }

#ifndef NLOG
    parser.log();
#endif

    IRGenerator ir_generator(parser);
    if (!ir_generator.get_success()) {
        std::cout << "Exiting due to compile error." << '\n';
        return;
    }

#ifndef NLOG
    ir_generator.log();
#endif

    Compiler compiler(ir_generator, out);
    if (!compiler.get_success()) {
        std::cout << "Exiting due to compile error." << '\n';
        return;
    }

#ifndef NLOG
    // End program
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << std::fixed;
    std::cout << "[Done] Program compiled in " << elapsed.count() / 1000.0 << " seconds" << '\n';
    std::cout << '\n';
    std::cout << " -- Compile result -- " << '\n';  
#endif
    if (should_run) compiler.run();
}