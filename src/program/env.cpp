#include <program/env.h>

Env::Env() {}

Env::~Env() {
    for (const auto &object : registry.get_objects()) {
        delete object.second;
    }
}

Env& Env::get_instance() {
    static Env instance;
    return instance;
}

void Env::build() {
    if (system == SYSTEM_WIN64) {
        const nlohmann::json project = Utils::read_json("project.json");

        std::string src_dir = project["detail"]["src"].get<std::string>();
        std::string obj_dir = project["detail"]["out"].get<std::string>();

        Utils::replace_slashes(src_dir);
        Utils::replace_slashes(obj_dir);

        std::filesystem::path path{obj_dir};
        std::filesystem::create_directories(path);

        const auto sources = Utils::get_sources(src_dir);

        for (const auto &id : sources) {
            request(id);
        }

        // link
        std::string obj_all_path = "";
        for (const auto &src_id : sources) {
            obj_all_path += obj_dir + src_id + ".o ";
        }

        std::string id = project["project"]["id"].get<std::string>();

        Utils::run_cmd("gcc.exe -m64 -g " + obj_all_path + "-o " + obj_dir + id + ".exe");

        for (const auto &src_id : sources) {
            Utils::run_cmd("del \"" + obj_dir + src_id + ".o\"");
        }
    } else {
        throw std::runtime_error("System architechture not supported.");
    }
}

void Env::run() {
    if (system == SYSTEM_WIN64) {
        const nlohmann::json project = Utils::read_json("project.json");
        std::string id = project["project"]["id"].get<std::string>();
        std::string obj_dir = project["detail"]["out"].get<std::string>();
        Utils::replace_slashes(obj_dir);
        Utils::run_cmd(obj_dir + id + ".exe");
    } else {
        throw std::runtime_error("System architechture not supported.");
    }
}

Object* Env::request(const std::string &id) {
    if (!registry.is_object_compiled(id)) {
        const nlohmann::json project = Utils::read_json("project.json");
        std::string obj_dir = project["detail"]["out"].get<std::string>();
        
        Object *object = new Object(id, obj_dir);
        object->build();

        registry.push_object(id, object);
    }

    return registry.get_object(id);
}

Env::Registry::Registry() {}

void Env::Registry::push_object(const std::string &id, Object *object) {
    objects.emplace(id, object);
}

const bool Env::Registry::is_object_compiled(const std::string &id) {
    return objects.count(id);
}

Object* Env::Registry::get_object(const std::string &id) {
    if (is_object_compiled(id)) {
        return objects.at(id);
    } else {
        return nullptr;
    }
}

std::unordered_map<std::string, Object*>& Env::Registry::get_objects() {
    return objects;
}