#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>

std::vector<std::string> ParseInput(const std::string& input);
std::string JoinArgs(const std::vector<std::string>& args, size_t startIndex);

#endif