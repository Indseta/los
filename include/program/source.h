#pragma once

#include <string>

class Source {
	public:
		Source(const std::string &fp);

		const std::string& get();

	private:
		const std::string raw;
};