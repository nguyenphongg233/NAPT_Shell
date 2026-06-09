#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>

void LaunchExternal(std::string cmd, bool background);
void ExecutePipeline(std::string cmd1, std::string cmd2, bool isBackground);

#endif // EXECUTOR_H
