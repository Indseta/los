#include <nlohmann/json.hpp>
#include <whereami/whereami.h>

#include <program/console.h>

#include <program/lexer.h>
#include <program/parser.h>
#include <program/interpreter.h>

#include <chrono>
#include <cstring>
#include <istream>
#include <iostream>
#include <string>
#include <vector>


const std::string get_base_dir() {
    std::string base_dir;
    {
        int length, dirname_length;
        length = wai_getExecutablePath(0, 0, 0);

        char *path = (char*) malloc(length + 1);
        wai_getExecutablePath(path, length, &dirname_length);

        base_dir = path;
    }

    size_t erase_n = 0;
    for (int i = base_dir.length(); i > 0; --i) {
        if (base_dir[i] == '\\') {
            erase_n = base_dir.length() - i;
            break;
        }
    }

    base_dir.erase(base_dir.length() - erase_n - 3);

    return base_dir;
}


const std::string base_dir = get_base_dir();


const nlohmann::json read_json(const std::string json_path) {
    std::ifstream f(base_dir + json_path);
    const nlohmann::json properties = nlohmann::json::parse(f);
    f.close();

    return properties;
}


int main(int argc, char *argv[]) {
    // cmd::clear();

    if (argc < 2) {
        std::cout << "Missing required arguments" << '\n';
        std::cout << "Run 'ast -help' for a list of commands" << '\n';
        return EXIT_FAILURE;
    }

    const nlohmann::json properties = read_json("src/resources/ast.json");
    const std::string version = properties["ast"]["version"].get<std::string>();

    if (strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "-h") == 0) {
        std::cout << "usage: 'ast [option] ...'" << '\n';
        std::cout << "options:" << '\n';
        std::cout << "\t<path_to_file.sf> : run program" << '\n';
        std::cout << "\t-v or -version : show version" << '\n';
        std::cout << "\t-h or -help : show help information" << '\n';
    } else if (strcmp(argv[1], "-version") == 0 || strcmp(argv[1], "-v") == 0) {
        std::cout << "ast v" << version << '\n';
    } else {
        // Start program
        auto start = std::chrono::high_resolution_clock::now();

        const Parser parser(argv[1]);
		Interpreter interpreter;
		interpreter.interpret(parser.ast);
		interpreter.debug_print();

        // End program
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;

        cmd::ostream::newline();

        std::cout << "[Done] Program finished in " << elapsed.count() / 1000.0 << " seconds" << '\n';

        // Flush console
        cmd::ostream::fbuffer();
    }

    return EXIT_SUCCESS;
}