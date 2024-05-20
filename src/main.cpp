#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

#include <nlohmann/json.hpp>
#include <whereami/whereami.h>

#define NLOG
#include <program/environment.h>

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

void split_fp(const std::string &org, std::string &dir, std::string &file) {
    dir = "";
    file = "";

    bool is_dir = false;
    for (int i = org.size() - 1; i >= 0; --i) {
        const auto &c = org[i];
        if (c == '/' && !is_dir) {
            is_dir = true;
            continue;
        }

        if (is_dir) dir = c + dir;
        else file = c + file;
    }
}

const std::string capitalize(const std::string &value) {
    std::string capitalized = "";
    bool should_capitalize = true;
    for (const auto &c : value) {
        if (should_capitalize) {
            if (c == '_' || c == '-') continue;
            else {
                capitalized += std::toupper(c);
                should_capitalize = false;
            }
        }
        else {
            if (c == '_' || c == '-') {
                capitalized += ' ';
                should_capitalize = true;
            }
            else {
                capitalized += c;
            }
        }
    }
    return capitalized;
}

const nlohmann::json read_json(const std::string json_path, bool rel = true) {
    std::ifstream f((rel ? "" : base_dir) + json_path);
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

    const nlohmann::json properties = read_json("src/resources/los.json", false);
    const std::string version = properties["los"]["version"].get<std::string>();

    if (strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "-h") == 0) {
        std::cout << "usage: 'los [option] ...'" << '\n';
        std::cout << "options:" << '\n';
        std::cout << "\t<path_to_file> : run program" << '\n';
        std::cout << "\t-v or -version : show version" << '\n';
        std::cout << "\t-h or -help : show help information" << '\n';
    } else if (strcmp(argv[1], "-version") == 0 || strcmp(argv[1], "-v") == 0) {
        std::cout << "los v" << version << '\n';
    } else if (strcmp(argv[1], "new") == 0 && argc > 2) {
        std::string id = argv[2];
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
        project_ofs << '\t' << '\t' << "\"name\": \"" + capitalize(id) + "\"," << '\n';
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

    } else if (strcmp(argv[1], "build") == 0) {
        const nlohmann::json project = read_json("project.json");

        const std::string src_org = project["detail"]["src"].get<std::string>();
        const std::string out_org = project["detail"]["out"].get<std::string>();

        std::string src_dir, src_main;
        std::string out_dir, out_main;

        split_fp(src_org, src_dir, src_main);
        split_fp(out_org, out_dir, out_main);

        std::filesystem::path path{out_dir};
        path /= out_main;
        std::filesystem::create_directories(path.parent_path());

        // Start program
        Environment environment;
        try {
            environment.run(src_org, out_org, false);
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    } else if (strcmp(argv[1], "run") == 0) {
        const nlohmann::json project = read_json("project.json");

        const std::string src_org = project["detail"]["src"].get<std::string>();
        const std::string out_org = project["detail"]["out"].get<std::string>();

        std::string src_dir, src_main;
        std::string out_dir, out_main;

        split_fp(src_org, src_dir, src_main);
        split_fp(out_org, out_dir, out_main);

        std::filesystem::path path{out_dir};
        path /= out_main;
        std::filesystem::create_directories(path.parent_path());

        // Start program
        Environment environment;
        try {
            environment.run(src_org, out_org);
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
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
            environment.run(fp, fp);
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}