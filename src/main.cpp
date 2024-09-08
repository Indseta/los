#include <cstring>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

#define NLOG
#include <program/env.h>
#include <program/utils.h>

void help() {
    std::cout << "usage: 'los [option] ...'" << '\n';
    std::cout << "options:" << '\n';
    std::cout << "\t<path_to_file> : run program" << '\n';
    std::cout << "\t-v or -version : show version" << '\n';
    std::cout << "\t-h or -help : show help information" << '\n';
}

void version() {
    const nlohmann::json properties = Utils::read_json("src/resources/los.json", false);
    const std::string version = properties["los"]["version"].get<std::string>();
    std::cout << "los v" << version << '\n';
}

void new_project(const std::string &id) {
    std::filesystem::create_directory("./" + id);
    std::filesystem::create_directory("./" + id + "/src");

    std::ofstream src_ofs(id + "/src/main.los");
    src_ofs << "void main() {" << '\n';
    src_ofs << '\t' << "println(\"Hello world\")" << '\n'; 
    src_ofs << "}" << '\n';
    src_ofs.close();

    std::ofstream project_ofs(id + "/project.json");
    project_ofs << "{" << '\n';
    project_ofs << '\t' << "\"project\": {" << '\n';
    project_ofs << '\t' << '\t' << "\"id\": \"" + id + "\"," << '\n';
    project_ofs << '\t' << '\t' << "\"name\": \"" + Utils::capitalize(id) + "\"," << '\n';
    project_ofs << '\t' << '\t' << "\"version\": \"1.0.0\"" << '\n';
    project_ofs << '\t' << "}," << '\n';
    project_ofs << '\t' << "\"detail\": {" << '\n';
    project_ofs << '\t' << '\t' << "\"src\": \"./src/main\"," << '\n';
    project_ofs << '\t' << '\t' << "\"out\": \"./bin/main\"," << '\n';
    project_ofs << '\t' << '\t' << "\"worker\": 0" << '\n';
    project_ofs << '\t' << "}," << '\n';
    project_ofs << '\t' << "\"libs\": []" << '\n';
    project_ofs << "}" << '\n';
    project_ofs.close();
}

int build() {
    try {
        auto &env = Env::get_instance();
        env.build();
        return EXIT_SUCCESS;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}

int run() {
    try {
        auto &env = Env::get_instance();
        env.build();
        env.run();
        return EXIT_SUCCESS;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
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

    if (strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "-h") == 0) {
        help();
    } else if (strcmp(argv[1], "-version") == 0 || strcmp(argv[1], "-v") == 0) {
        version();
    } else if (strcmp(argv[1], "new") == 0 && argc > 2) {
        new_project(argv[2]);
    } else if (strcmp(argv[1], "build") == 0 ) {
        return build();
    } else if (strcmp(argv[1], "run") == 0) {
        return run();
    } else {
        std::string full_cmd = "";
        for (int i = 0; i < argc; ++i) {
            full_cmd += argv[i];
            if (i < argc - 1) {
                full_cmd += " ";
            }
        }
        std::cerr << "Unknown command: '" << full_cmd << "'" << '\n';
    }

    return EXIT_SUCCESS;
}