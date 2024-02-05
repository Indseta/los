#include <program/source.h>

Source::Source(const std::string &fp) : raw(read_source(fp)) {}

const std::string Source::read_source(const std::string &fp) {
    std::ifstream file(fp);

    if (!file.is_open()) {
        std::cerr << "Failed to open file" << '\n';
        return "";
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    file.close();
    return ss.str();
}

const std::string& Source::get() const {
	return raw;
}