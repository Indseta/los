#include <nlohmann/json.hpp>
#include <whereami/whereami.h>

#define NLOG
#include <program/environment.h>

#include <cstring>
#include <istream>
#include <iostream>
#include <string>
#include <stdexcept>
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
// #ifdef _WIN32
//     std::system("cls");
// #else
//     std::system("clear");
// #endif

    if (argc < 2) {
        std::cout << "Missing required arguments" << '\n';
        std::cout << "Run 'los -help' for a list of commands" << '\n';
        return EXIT_FAILURE;
    }

    const nlohmann::json properties = read_json("src/resources/los.json");
    const std::string version = properties["los"]["version"].get<std::string>();

    if (strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "-h") == 0) {
        std::cout << "usage: 'los [option] ...'" << '\n';
        std::cout << "options:" << '\n';
        std::cout << "\t<path_to_file> : run program" << '\n';
        std::cout << "\t-v or -version : show version" << '\n';
        std::cout << "\t-h or -help : show help information" << '\n';
    } else if (strcmp(argv[1], "-version") == 0 || strcmp(argv[1], "-v") == 0) {
        std::cout << "los v" << version << '\n';
    } else {
        // Start program
        Environment environment;
        try {
            std::string fp = argv[1];
            std::string ext = ".los";

            size_t pos = fp.find(ext);
            if (pos != std::string::npos) {
                fp.erase(pos, ext.length());
            }
            environment.run(fp);
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}