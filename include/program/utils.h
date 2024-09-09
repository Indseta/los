#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <whereami/whereami.h>

class Utils {
public:
    static const std::string get_base_dir();
    static const std::string base_dir;

    static const std::vector<std::string> get_sources(const std::string &folder);

    static void replace_slashes(std::string &path);

    static const std::string src_id_to_path(const std::string &id);

    static const std::string capitalize(const std::string &value);

    static const nlohmann::json read_json(const std::string json_path, bool rel = true);

    static int run_cmd(const std::string &cmd);
};