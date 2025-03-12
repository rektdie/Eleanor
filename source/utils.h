#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <sstream>

std::vector<std::string> split(std::string_view str, char delim) {
    std::vector<std::string> results;

    std::istringstream stream(std::string{str});

    for (std::string token{}; std::getline(stream, token, delim);) {
        if (token.empty()) continue;

        results.push_back(token);
    }

    return results;
}

int parseSquare(std::string_view str) {
    constexpr std::string_view files = "abcdefgh";

    int file = files.find(str[0]);
    int rank = str[1] - '0';

    return file + rank * 8;
}
