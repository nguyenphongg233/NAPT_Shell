#include <windows.h>
#include <tlhelp32.h>

#include <conio.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace {
string trim(const string& value) {
    size_t start = 0;
    while (start < value.size() && isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return value.substr(start, end - start);
}

string toLowerCopy(string value) {
    transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });
    return value;
}

bool startsWith(const string& value, const string& prefix) {
    if (prefix.size() > value.size()) {
        return false;
    }
    return equal(prefix.begin(), prefix.end(), value.begin());
}

bool startsWithInsensitive(const string& value, const string& prefix) {
    if (prefix.size() > value.size()) {
        return false;
    }

    for (size_t i = 0; i < prefix.size(); ++i) {
        if (tolower(static_cast<unsigned char>(value[i])) !=
            tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }

    return true;
}

string longestCommonPrefix(const vector<string>& values) {
    if (values.empty()) {
        return "";
    }

    string prefix = values[0];
    for (size_t i = 1; i < values.size() && !prefix.empty(); ++i) {
        size_t matchLength = 0;
        while (matchLength < prefix.size() &&
               matchLength < values[i].size() &&
               prefix[matchLength] == values[i][matchLength]) {
            ++matchLength;
        }
        prefix = prefix.substr(0, matchLength);
    }

    return prefix;
}

vector<string> tokenize(const string& input) {
    vector<string> tokens;
    string current;
    bool inQuotes = false;

    for (char ch : input) {
        if (ch == '"') {
            inQuotes = !inQuotes;
            continue;
        }

        if (!inQuotes && isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(ch);
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

string statusToString(int statusCode) {
    switch (statusCode) {
        case 0:
            return "running";
        case 1:
            return "suspended";
        case 2:
            return "finished";
        case 3:
            return "terminated";
        default:
            return "unknown";
    }
}
}

class TinyShell {
private:
    enum ProcessStatus {
        Running = 0,
        Suspended = 1,
        Finished = 2,
        Terminated = 3,
    };

    struct ProcessInfo {
        DWORD processId = 0;
        HANDLE processHandle = nullptr;
        string commandLine;
        ProcessStatus status = Running;
        bool background = false;
        DWORD exitCode = STILL_ACTIVE;
    };

    struct ParsedCommand {
        string rawLine;
        string name;
        string rawArgs;
        vector<string> args;
    };

    vector<ProcessInfo> processList;
    vector<string> commandHistory;
    DWORD foregroundProcessGroupId = 0;

    static TinyShell* activeShell;

    static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
        if (activeShell == nullptr) {
            return FALSE;
        }
        return activeShell->handleConsoleSignal(ctrlType);
    }

    BOOL handleConsoleSignal(DWORD ctrlType) {
        if (ctrlType != CTRL_C_EVENT && ctrlType != CTRL_BREAK_EVENT) {
            return FALSE;
        }

        if (foregroundProcessGroupId == 0) {
            cout << "\nNo foreground process to interrupt.\n";
            return TRUE;
        }

        if (GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, foregroundProcessGroupId)) {
            cout << "\nInterrupt signal sent to foreground process group "
                 << foregroundProcessGroupId << "\n";
        } else {
            cout << "\nFailed to interrupt foreground process group "
                 << foregroundProcessGroupId << "\n";
        }
        return TRUE;
    }

    vector<string> getBuiltinCommands() const {
        return {
            "help", "exit", "cls", "date", "time", "dir", "cd",
            "path", "addpath", "history", "fg", "foreground",
            "bg", "background", "bat", "list", "kill", "stop", "resume"
        };
    }

    void redrawInputLine(const string& prompt, const string& line) const {
        cout << "\r" << prompt << line << " \r" << prompt << line << flush;
    }

    void completeCommandName(string& line, const string& prompt) const {
        string prefix = toLowerCopy(line);
        const vector<string> builtins = getBuiltinCommands();

        vector<string> matches;
        for (const string& command : builtins) {
            if (startsWith(command, prefix)) {
                matches.push_back(command);
            }
        }

        if (matches.empty()) {
            return;
        }

        if (matches.size() == 1) {
            line = matches[0] + " ";
            redrawInputLine(prompt, line);
            return;
        }

        string prefixFromMatches = longestCommonPrefix(matches);
        if (prefixFromMatches.size() > prefix.size()) {
            line = prefixFromMatches;
        }

        cout << "\n";
        for (const string& match : matches) {
            cout << match << "  ";
        }
        cout << "\n";
        redrawInputLine(prompt, line);
    }

    bool isPathCompletionCommand(const string& commandName) const {
        return commandName == "cd" || commandName == "dir" || commandName == "bat";
    }

    void completePathToken(string& line, const string& prompt) const {
        const size_t firstSpace = line.find(' ');
        if (firstSpace == string::npos) {
            return;
        }

        string commandName = toLowerCopy(trim(line.substr(0, firstSpace)));
        if (!isPathCompletionCommand(commandName)) {
            return;
        }

        const size_t tokenStart = line.find_last_of(' ') + 1;
        if (tokenStart >= line.size()) {
            return;
        }

        string token = line.substr(tokenStart);
        bool quoted = false;
        if (!token.empty() && token[0] == '"') {
            quoted = true;
            token = token.substr(1);
        }

        const size_t separatorIndex = token.find_last_of("\\/");
        const string basePath = (separatorIndex == string::npos) ? "" : token.substr(0, separatorIndex + 1);
        const string namePrefix = (separatorIndex == string::npos) ? token : token.substr(separatorIndex + 1);

        string searchPattern = basePath.empty() ? "*" : basePath + "*";

        WIN32_FIND_DATAA findData = {};
        HANDLE handle = FindFirstFileA(searchPattern.c_str(), &findData);
        if (handle == INVALID_HANDLE_VALUE) {
            return;
        }

        struct PathCandidate {
            string name;
            bool isDirectory;
        };

        vector<PathCandidate> matches;
        do {
            string candidateName = findData.cFileName;
            if (candidateName == "." || candidateName == "..") {
                continue;
            }

            if (!startsWithInsensitive(candidateName, namePrefix)) {
                continue;
            }

            bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            matches.push_back({candidateName, isDirectory});
        } while (FindNextFileA(handle, &findData));

        FindClose(handle);

        if (matches.empty()) {
            return;
        }

        if (matches.size() == 1) {
            string replacement = basePath + matches[0].name;
            if (matches[0].isDirectory) {
                replacement += "\\";
            }

            if (quoted) {
                replacement = "\"" + replacement;
            }

            line = line.substr(0, tokenStart) + replacement;
            if (!matches[0].isDirectory) {
                line += " ";
            }

            redrawInputLine(prompt, line);
            return;
        }

        vector<string> matchNames;
        for (const auto& match : matches) {
            matchNames.push_back(match.name);
        }

        string commonPrefix = longestCommonPrefix(matchNames);
        if (commonPrefix.size() > namePrefix.size()) {
            string replacement = basePath + commonPrefix;
            if (quoted) {
                replacement = "\"" + replacement;
            }
            line = line.substr(0, tokenStart) + replacement;
        }

        cout << "\n";
        for (const auto& match : matches) {
            cout << match.name;
            if (match.isDirectory) {
                cout << "\\";
            }
            cout << "  ";
        }
        cout << "\n";
        redrawInputLine(prompt, line);
    }

    string readInputLine(const string& prompt) const {
        cout << prompt << flush;
        string line;

        while (true) {
            int key = _getch();

            if (key == '\r') {
                cout << "\n";
                return line;
            }

            if (key == '\b') {
                if (!line.empty()) {
                    line.pop_back();
                    cout << "\b \b" << flush;
                }
                continue;
            }

            if (key == '\t') {
                if (line.find(' ') == string::npos) {
                    completeCommandName(line, prompt);
                } else {
                    completePathToken(line, prompt);
                }
                continue;
            }

            if (key == 0 || key == 224) {
                (void)_getch();
                continue;
            }

            if (isprint(static_cast<unsigned char>(key))) {
                char typed = static_cast<char>(key);
                line.push_back(typed);
                cout << typed << flush;
            }
        }
    }

    ParsedCommand parseCommand(const string& input) const {
        ParsedCommand parsed;
        parsed.rawLine = trim(input);
        if (parsed.rawLine.empty()) {
            return parsed;
        }

        size_t index = 0;
        bool inQuotes = false;
        while (index < parsed.rawLine.size()) {
            char ch = parsed.rawLine[index];
            if (ch == '"') {
                inQuotes = !inQuotes;
            } else if (!inQuotes && isspace(static_cast<unsigned char>(ch))) {
                break;
            }
            ++index;
        }

        parsed.name = parsed.rawLine.substr(0, index);
        parsed.name.erase(remove(parsed.name.begin(), parsed.name.end(), '"'), parsed.name.end());
        parsed.name = toLowerCopy(parsed.name);
        parsed.rawArgs = trim(parsed.rawLine.substr(index));
        parsed.args = tokenize(parsed.rawArgs);
        return parsed;
    }

    void refreshProcesses() {
        for (auto& process : processList) {
            if (process.processHandle == nullptr) {
                continue;
            }

            if (process.status == Finished || process.status == Terminated) {
                continue;
            }

            DWORD exitCode = STILL_ACTIVE;
            if (!GetExitCodeProcess(process.processHandle, &exitCode)) {
                continue;
            }

            process.exitCode = exitCode;
            if (exitCode != STILL_ACTIVE) {
                process.status = Finished;
            }
        }
    }

    ProcessInfo* findProcess(DWORD processId) {
        for (auto& process : processList) {
            if (process.processId == processId) {
                return &process;
            }
        }
        return nullptr;
    }

    bool suspendProcessThreads(DWORD processId) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return false;
        }

        THREADENTRY32 entry = {};
        entry.dwSize = sizeof(entry);
        bool suspendedAnyThread = false;

        if (Thread32First(snapshot, &entry)) {
            do {
                if (entry.th32OwnerProcessID != processId) {
                    continue;
                }

                HANDLE threadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
                if (threadHandle == nullptr) {
                    continue;
                }

                if (SuspendThread(threadHandle) != static_cast<DWORD>(-1)) {
                    suspendedAnyThread = true;
                }

                CloseHandle(threadHandle);
            } while (Thread32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return suspendedAnyThread;
    }

    bool resumeProcessThreads(DWORD processId) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return false;
        }

        THREADENTRY32 entry = {};
        entry.dwSize = sizeof(entry);
        bool resumedAnyThread = false;

        if (Thread32First(snapshot, &entry)) {
            do {
                if (entry.th32OwnerProcessID != processId) {
                    continue;
                }

                HANDLE threadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
                if (threadHandle == nullptr) {
                    continue;
                }

                DWORD suspendCount = ResumeThread(threadHandle);
                while (suspendCount > 0 && suspendCount != static_cast<DWORD>(-1)) {
                    resumedAnyThread = true;
                    suspendCount = ResumeThread(threadHandle);
                }
                if (suspendCount == 0) {
                    resumedAnyThread = true;
                }

                CloseHandle(threadHandle);
            } while (Thread32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return resumedAnyThread;
    }

    bool createProcessRecord(const string& fullCommand, bool background, ProcessInfo& processInfo) {
        STARTUPINFOA startupInfo = {};
        PROCESS_INFORMATION processInformation = {};
        startupInfo.cb = sizeof(startupInfo);

        vector<char> commandBuffer(fullCommand.begin(), fullCommand.end());
        commandBuffer.push_back('\0');

        DWORD creationFlags = CREATE_NEW_PROCESS_GROUP;
        if (background) {
            creationFlags |= CREATE_NEW_CONSOLE;
        }

        BOOL created = CreateProcessA(
            nullptr,
            commandBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            creationFlags,
            nullptr,
            nullptr,
            &startupInfo,
            &processInformation
        );

        if (!created) {
            DWORD errorCode = GetLastError();
            if (errorCode == ERROR_FILE_NOT_FOUND || errorCode == ERROR_PATH_NOT_FOUND) {
                cout << "Invalid command or syntax: " << fullCommand << "\n";
                cout << "Type 'help' to see supported commands.\n";
            } else {
                cout << "Unable to execute command. Error code: " << errorCode << "\n";
            }
            return false;
        }

        processInfo.processId = processInformation.dwProcessId;
        processInfo.processHandle = processInformation.hProcess;
        processInfo.commandLine = fullCommand;
        processInfo.status = Running;
        processInfo.background = background;
        processInfo.exitCode = STILL_ACTIVE;

        CloseHandle(processInformation.hThread);
        return true;
    }

    void executeExternalCommand(const string& fullCommand, bool background) {
        if (trim(fullCommand).empty()) {
            cout << "Missing command to execute.\n";
            return;
        }

        ProcessInfo processInfo;
        if (!createProcessRecord(fullCommand, background, processInfo)) {
            return;
        }

        processList.push_back(processInfo);
        ProcessInfo& storedProcess = processList.back();

        if (background) {
            cout << "Background process started (PID: " << storedProcess.processId << ")\n";
            return;
        }

        foregroundProcessGroupId = storedProcess.processId;
        WaitForSingleObject(storedProcess.processHandle, INFINITE);
        foregroundProcessGroupId = 0;
        refreshProcesses();
    }

    void printHelp() const {
        cout << "Available commands:\n";
        cout << "help                 - Show this help message\n";
        cout << "exit                 - Exit the shell\n";
        cout << "cls                  - Clear the console\n";
        cout << "date                 - Show current date\n";
        cout << "time                 - Show current time\n";
        cout << "dir [path]           - List directory contents\n";
        cout << "cd [path]            - Change current directory\n";
        cout << "path                 - Show current PATH\n";
        cout << "addpath <new_path>   - Append a directory to PATH\n";
        cout << "history              - Show command history\n";
        cout << "fg <command>         - Run command in foreground\n";
        cout << "bg <command>         - Run command in background\n";
        cout << "bat <file.bat>       - Execute a batch file\n";
        cout << "list                 - List tracked processes with status\n";
        cout << "kill <pid|all>       - Terminate a tracked process\n";
        cout << "stop <pid>           - Suspend a tracked process\n";
        cout << "resume <pid>         - Resume a suspended process\n";
        cout << "Any non-builtin command runs as an external process in foreground mode.\n";
    }

    void exitShell() {
        refreshProcesses();
        for (auto& process : processList) {
            if (process.processHandle == nullptr) {
                continue;
            }

            if (process.status == Running || process.status == Suspended) {
                TerminateProcess(process.processHandle, 0);
                process.status = Terminated;
            }

            CloseHandle(process.processHandle);
            process.processHandle = nullptr;
        }

        cout << "Exiting TinyShell...\n";
        ExitProcess(0);
    }

    void clearConsole() const {
        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (consoleHandle == INVALID_HANDLE_VALUE) {
            return;
        }

        CONSOLE_SCREEN_BUFFER_INFO info = {};
        if (!GetConsoleScreenBufferInfo(consoleHandle, &info)) {
            return;
        }

        DWORD cellCount = info.dwSize.X * info.dwSize.Y;
        DWORD written = 0;
        COORD home = {0, 0};

        FillConsoleOutputCharacterA(consoleHandle, ' ', cellCount, home, &written);
        FillConsoleOutputAttribute(consoleHandle, info.wAttributes, cellCount, home, &written);
        SetConsoleCursorPosition(consoleHandle, home);
    }

    void showDate() const {
        SYSTEMTIME systemTime = {};
        GetLocalTime(&systemTime);
        cout << "Current date: "
             << systemTime.wDay << "/"
             << systemTime.wMonth << "/"
             << systemTime.wYear << "\n";
    }

    void showTime() const {
        SYSTEMTIME systemTime = {};
        GetLocalTime(&systemTime);
        cout << "Current time: "
             << systemTime.wHour << ":"
             << systemTime.wMinute << ":"
             << systemTime.wSecond << "\n";
    }

    void showPath() const {
        char buffer[32767];
        DWORD size = GetEnvironmentVariableA("PATH", buffer, sizeof(buffer));
        if (size == 0 || size >= sizeof(buffer)) {
            cout << "Failed to get PATH variable.\n";
            return;
        }

        cout << "Current PATH:\n" << buffer << "\n";
    }

    void addPath(const vector<string>& args) const {
        if (args.empty()) {
            cout << "Usage: addpath <new_path>\n";
            return;
        }

        char buffer[32767];
        DWORD size = GetEnvironmentVariableA("PATH", buffer, sizeof(buffer));
        string updatedPath = (size > 0 && size < sizeof(buffer))
            ? string(buffer) + ";" + args[0]
            : args[0];

        if (SetEnvironmentVariableA("PATH", updatedPath.c_str())) {
            cout << "Added to PATH: " << args[0] << "\n";
        } else {
            cout << "Failed to update PATH.\n";
        }
    }

    void showHistory() const {
        if (commandHistory.empty()) {
            cout << "Command history is empty.\n";
            return;
        }

        for (size_t index = 0; index < commandHistory.size(); ++index) {
            cout << index + 1 << ": " << commandHistory[index] << "\n";
        }
    }

    void listProcesses() {
        refreshProcesses();
        if (processList.empty()) {
            cout << "No tracked processes.\n";
            return;
        }

        cout << "PID\tMode\tStatus\t\tCommand\n";
        cout << "---\t----\t------\t\t-------\n";
        for (const auto& process : processList) {
            cout << process.processId << "\t"
                 << (process.background ? "bg" : "fg") << "\t"
                 << statusToString(process.status) << "\t\t"
                 << process.commandLine << "\n";
        }
    }

    void killProcess(const vector<string>& args) {
        refreshProcesses();
        if (args.empty()) {
            cout << "Usage: kill <pid|all>\n";
            return;
        }

        if (args[0] == "all") {
            for (auto& process : processList) {
                if (process.processHandle == nullptr) {
                    continue;
                }
                if (process.status == Running || process.status == Suspended) {
                    TerminateProcess(process.processHandle, 1);
                    process.status = Terminated;
                    process.exitCode = 1;
                }
            }
            cout << "Termination signal sent to all tracked processes.\n";
            return;
        }

        DWORD processId = stoul(args[0]);
        ProcessInfo* process = findProcess(processId);
        if (process == nullptr || process->processHandle == nullptr) {
            cout << "Process " << processId << " not found.\n";
            return;
        }

        if (TerminateProcess(process->processHandle, 1)) {
            process->status = Terminated;
            process->exitCode = 1;
            cout << "Process " << processId << " terminated.\n";
        } else {
            cout << "Failed to terminate process " << processId << ".\n";
        }
    }

    void stopProcess(const vector<string>& args) {
        refreshProcesses();
        if (args.empty()) {
            cout << "Usage: stop <pid>\n";
            return;
        }

        DWORD processId = stoul(args[0]);
        ProcessInfo* process = findProcess(processId);
        if (process == nullptr) {
            cout << "Process " << processId << " not found.\n";
            return;
        }
        if (process->status == Finished || process->status == Terminated) {
            cout << "Process " << processId << " is no longer running.\n";
            return;
        }

        if (suspendProcessThreads(processId)) {
            process->status = Suspended;
            cout << "Process " << processId << " suspended.\n";
        } else {
            cout << "Failed to suspend process " << processId << ".\n";
        }
    }

    void resumeProcess(const vector<string>& args) {
        refreshProcesses();
        if (args.empty()) {
            cout << "Usage: resume <pid>\n";
            return;
        }

        DWORD processId = stoul(args[0]);
        ProcessInfo* process = findProcess(processId);
        if (process == nullptr) {
            cout << "Process " << processId << " not found.\n";
            return;
        }
        if (process->status == Finished || process->status == Terminated) {
            cout << "Process " << processId << " is no longer running.\n";
            return;
        }

        if (resumeProcessThreads(processId)) {
            process->status = Running;
            cout << "Process " << processId << " resumed.\n";
        } else {
            cout << "Failed to resume process " << processId << ".\n";
        }
    }

    void runBatchFile(const string& rawArgs) {
        if (trim(rawArgs).empty()) {
            cout << "Usage: bat <file.bat>\n";
            return;
        }
        executeExternalCommand("cmd.exe /c " + rawArgs, false);
    }

    void runDirectoryCommand(const string& rawArgs) {
        string command = "cmd.exe /c dir";
        if (!trim(rawArgs).empty()) {
            command += " " + rawArgs;
        }
        executeExternalCommand(command, false);
    }

    void changeDirectory(const vector<string>& args) const {
        if (args.empty()) {
            char buffer[MAX_PATH];
            DWORD size = GetCurrentDirectoryA(MAX_PATH, buffer);
            if (size == 0 || size > MAX_PATH) {
                cout << "Failed to get current directory.\n";
                return;
            }
            cout << buffer << "\n";
            return;
        }

        if (SetCurrentDirectoryA(args[0].c_str())) {
            char buffer[MAX_PATH];
            DWORD size = GetCurrentDirectoryA(MAX_PATH, buffer);
            if (size == 0 || size > MAX_PATH) {
                cout << "Directory changed successfully.\n";
            } else {
                cout << "Current directory changed to: " << buffer << "\n";
            }
        } else {
            cout << "Failed to change directory.\n";
        }
    }

    void runParsedCommand(const ParsedCommand& command) {
        if (command.rawLine.empty()) {
            return;
        }

        commandHistory.push_back(command.rawLine);

        if (command.name == "help") {
            printHelp();
        } else if (command.name == "exit") {
            exitShell();
        } else if (command.name == "cls") {
            clearConsole();
        } else if (command.name == "date") {
            showDate();
        } else if (command.name == "time") {
            showTime();
        } else if (command.name == "dir") {
            runDirectoryCommand(command.rawArgs);
        } else if (command.name == "cd") {
            changeDirectory(command.args);
        } else if (command.name == "path") {
            showPath();
        } else if (command.name == "addpath") {
            addPath(command.args);
        } else if (command.name == "history") {
            showHistory();
        } else if (command.name == "fg" || command.name == "foreground") {
            executeExternalCommand(command.rawArgs, false);
        } else if (command.name == "bg" || command.name == "background") {
            executeExternalCommand(command.rawArgs, true);
        } else if (command.name == "bat") {
            runBatchFile(command.rawArgs);
        } else if (command.name == "list") {
            listProcesses();
        } else if (command.name == "kill") {
            killProcess(command.args);
        } else if (command.name == "stop") {
            stopProcess(command.args);
        } else if (command.name == "resume") {
            resumeProcess(command.args);
        } else {
            executeExternalCommand(command.rawLine, false);
        }
    }

public:
    TinyShell() {
        activeShell = this;
        SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
    }

    ~TinyShell() {
        SetConsoleCtrlHandler(consoleCtrlHandler, FALSE);
        for (auto& process : processList) {
            if (process.processHandle != nullptr) {
                CloseHandle(process.processHandle);
                process.processHandle = nullptr;
            }
        }
    }

    void run() {
        const string prompt = "~ ";
        while (true) {
            refreshProcesses();

            string line = readInputLine(prompt);

            ParsedCommand command = parseCommand(line);
            runParsedCommand(command);
        }
    }
};

TinyShell* TinyShell::activeShell = nullptr;

int main() {
    TinyShell shell;
    shell.run();
    return 0;
}
