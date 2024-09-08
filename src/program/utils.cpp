#include <program/utils.h>

const std::string Utils::get_base_dir() {
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

const std::string Utils::base_dir = get_base_dir();

const std::vector<std::string> Utils::get_sources(const std::string &folder) {
    std::vector<std::string> sources;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".los") {
            std::string relative_path = std::filesystem::relative(entry.path(), folder).string();

            relative_path = relative_path.substr(0, relative_path.find_last_of('.'));

            std::replace(relative_path.begin(), relative_path.end(), static_cast<char>(std::filesystem::path::preferred_separator), '.');

            sources.push_back(relative_path);
        }
    }

    return sources;
}

void Utils::replace_slashes(std::string &path) {
    std::string tmp = "";
    for (const auto &c : path) {
        if (c == '/') tmp += '\\';
        else tmp += c;
    }
    path = tmp;
}


const nlohmann::json project = Utils::read_json("project.json");

std::string src_dir = project["detail"]["src"].get<std::string>();
std::string obj_dir = project["detail"]["out"].get<std::string>();
const std::string Utils::src_id_to_path(const std::string &id) {
    std::string path = "";
    for (auto &c : id) {
        if (c == '.') path += '\\';
        else path += c;
    }
    path = src_dir + path + ".los";
    return path;
}

const std::string Utils::capitalize(const std::string &value) {
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

const nlohmann::json Utils::read_json(const std::string json_path, bool rel) {
    std::ifstream f((rel ? "" : base_dir) + json_path);
    const nlohmann::json properties = nlohmann::json::parse(f);
    f.close();

    return properties;
}

int Utils::run_cmd(const std::string &cmd) {
    return system(cmd.c_str());
}