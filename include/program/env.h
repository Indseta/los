#pragma once

#include <unordered_map>
#include <stdexcept>

#include <program/object.h>
#include <program/utils.h>

class Env {
private:
    enum System {
        SYSTEM_WIN32,
        SYSTEM_WIN64,
        SYSTEM_LINUX,
        SYSTEM_APPLE,
        SYSTEM_UNKNOWN,
    };

#if _WIN32
#if defined(_WIN64)
    static const System system = SYSTEM_WIN64;
#else
    static const System system = SYSTEM_WIN32;
#endif
#elif __linux__
    static const System system = SYSTEM_LINUX;
#elif __APPLE__
    static const System system = SYSTEM_APPLE;
#else
    static const System system = SYSTEM_UNKNOWN;
#endif

    struct Registry {
    public:
        Registry();

        void push_object(const std::string &id, Object *object);

        const bool is_object_compiled(const std::string &id);
        Object* get_object(const std::string &id);
        std::unordered_map<std::string, Object*>& get_objects();

    private:
        std::unordered_map<std::string, Object*> objects;
    };

public:
    static Env& get_instance();

    void build();
    void run();

    Object* request(const std::string &id);

    Registry registry;

private:
    Env();
    ~Env();
    Env(const Env&) = delete;
    Env& operator= (const Env&) = delete;
};