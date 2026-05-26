#include "../include/parser.h"
#include <sstream>

std::vector<std::string> ParseInput(const std::string& input) {
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string JoinArgs(const std::vector<std::string>& args, size_t startIndex) {
    std::string result = "";
    for (size_t i = startIndex; i < args.size(); ++i) {
        result += args[i];
        if (i < args.size() - 1) result += " ";
    }
    return result;
}