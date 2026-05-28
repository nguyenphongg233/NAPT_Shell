#include "../include/parser.h"
#include <sstream>

std::vector<std::string> ParseInput(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);
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