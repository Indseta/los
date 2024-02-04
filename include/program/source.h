#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Source {
public:
	Source(const std::string &fp);

	const std::string& get();

private:
	const std::string read_source(const std::string &fp);

	const std::string raw;
};