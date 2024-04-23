#include <program/source.h>

Source::Source(const std::string &fp) {
    success = false;
    read_source(fp);
}

void Source::read_source(const std::string &fp) {
    std::ifstream file(fp);

    if (file.is_open()) {
        success = true;
    } else {
        std::cerr << "Failed to open file" << '\n';
        return;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    file.close();
    raw = ss.str();
}

const std::string& Source::get() const {
    return raw;
}

const bool& Source::get_success() const {
    return success;
}