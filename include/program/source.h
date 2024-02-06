#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Source {
public:
    Source(const std::string &fp);
    const std::string& get() const;

private:
    void read_source(const std::string &fp);

    std::string raw;
};