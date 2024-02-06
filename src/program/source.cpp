#include <program/source.h>

Source::Source(const std::string &fp) {
    read_source(fp);
}

void Source::read_source(const std::string &fp) {
    std::ifstream file(fp);

    if (!file.is_open()) {
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