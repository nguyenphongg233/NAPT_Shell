#ifndef COMPLETION_H
#define COMPLETION_H

#include <string>
#include <vector>

// Get list of directories/files matching a partial path
std::vector<std::string> GetCompletions(const std::string& partialPath);

// Show completion suggestions for cd command
void ShowCompletionSuggestions(const std::string& currentInput);

// List files in directory (for ls command)
void ListDirectory(const std::string& path);

#endif // COMPLETION_H
