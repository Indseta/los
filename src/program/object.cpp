#include <program/object.h>
// #define NLOG

Object::Object(const std::string &src_id, const std::string &out_dir) : src_id(src_id), out_dir(out_dir) {
    success = true;
}

void Object::build() {
#ifndef NLOG
    auto start = std::chrono::high_resolution_clock::now();
#endif

    compile();

#ifndef NLOG
    // End program
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << std::fixed;
    std::cout << "[Done] Program compiled in " << elapsed.count() / 1000.0 << " seconds" << '\n';
    std::cout << '\n';
    std::cout << " -- Compile result -- " << '\n';
#endif
}

void Object::compile() {
    Source source(Utils::src_id_to_path(src_id));
    if (!source.get_success()) {
        std::cout << "Exiting due to read error." << '\n';
        success = false;
        return;
    }

    Lexer lexer(source);
    if (!lexer.get_success()) {
        std::cout << "Exiting due to lex error." << '\n';
        success = false;
        return;
    }

#ifndef NLOG
    lexer.log();
#endif

    Parser parser(lexer);
    if (!parser.get_success()) {
        std::cout << "Exiting due to parse error." << '\n';
        success = false;
        return;
    }

#ifndef NLOG
    parser.log();
#endif

    IRGenerator ir_generator(parser);
    if (!ir_generator.get_success()) {
        std::cout << "Exiting due to compile error." << '\n';
        success = false;
        return;
    }

#ifndef NLOG
    ir_generator.log();
#endif

    Compiler compiler(ir_generator, out_dir + src_id);
    if (!compiler.get_success()) {
        std::cout << "Exiting due to compile error." << '\n';
        success = false;
        return;
    }
}

void Object::log() {
}