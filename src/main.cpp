#include <program/console.h>
#include <program/lexer.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <sstream>
#include <vector>


std::string get_source(std::string path) {
    std::ifstream file("../src/" + path);

    if (!file.is_open()) {
        std::cerr << "Failed to open file" << '\n';
        return "";
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    file.close();

    return ss.str();
}


int main() {
    cmd::clear();

    // Start program
    auto start = std::chrono::high_resolution_clock::now();

    const auto lexer = Lexer("main.sf");
    for (const auto &token : lexer.get_tokens()) {
        std::cout << token << '\n';
    }

    // End program
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    cmd::ostream::newline();

    std::cout << "[Done] Program finished in " << elapsed.count() / 1000.0 << " seconds" << '\n';

    // Flush and pause console
    cmd::ostream::fbuffer();
    // cmd::pause();

    return 0;
}