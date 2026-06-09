#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>

void LaunchExternal(std::string cmd, bool background);
void ExecutePipeline(std::string cmd1, std::string cmd2, bool isBackground);
void ExecuteWithInputRedirection(std::string cmd, std::string inFile, bool isBackground);
void ExecuteWithOutputRedirection(std::string cmd, std::string outFile, bool isBackground);

#endif // EXECUTOR_H
