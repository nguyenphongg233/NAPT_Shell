#include<bits/stdc++.h>
#include<windows.h>
#pragma comment(lib, "kernel32.lib")

using namespace std;

class TinyShell{
private: 
    struct ProcessInfo {
        DWORD dwProcessId;    // ID tiến trình
        HANDLE hProcess;      // Handle để Kill/Stop
        HANDLE hThread;       // Handle để Resume/Suspend
        string cmdName;       // Tên lệnh (ví dụ: ping)
        bool isRunning;       // Trạng thái
    };

    vector<ProcessInfo> processList; // Danh sách các tiến trình đang chạy
    vector<string> commandHistory; // Lịch sử lệnh đã nhập
    vector<string> pathList; // Danh sách các đường dẫn đã thêm vào PATH

    vector<string> split(const string& str) {
        vector<string> tokens;
        stringstream ss(str);
        string words;
        while(ss >> words){
            if(words[0] == '"'){
                string temp;
                while(ss >> temp){
                    words += " " + temp;
                    if(temp.back() == '"') break;
                }
            }
            tokens.push_back(words);
        }
        return tokens;
    }

    struct Command{
        string name;
        vector<string> args;
    };

    
    void help(const vector<string> &args){
        cout << "Available commands:\n";
        cout << "help - Show this help message\n";
        cout << "exit - Exit the shell\n";
        cout << "cls - Clear the console\n";
        cout << "path - Show current PATH\n";
        cout << "addpath [new_path] - Add a new path to PATH\n";
        cout << "history - Show command history\n";
        cout << "Other commands will be executed as system commands if found in PATH.\n";

    }
    void exit(const vector<string> &args){
        cout << "Exiting TinyShell...\n";
        ExitProcess(0);
    }
    void cls(const vector<string> &args){
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD count;
        DWORD cellCount;
        COORD homeCoords = { 0, 0 };

        if (hConsole == INVALID_HANDLE_VALUE) return;

        // 1. Lấy thông tin về bộ đệm màn hình hiện tại (kích thước, vị trí con trỏ...)
        if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;

        // Tính tổng số ô (cells) cần phải xóa (Rộng * Cao)
        cellCount = csbi.dwSize.X * csbi.dwSize.Y;

        // 2. Ghi đè các ký tự khoảng trắng lên toàn bộ màn hình
        if (!FillConsoleOutputCharacter(
            hConsole,
            (TCHAR) ' ',     // Ký tự dùng để ghi đè (khoảng trắng)
            cellCount,       // Số lượng ô cần ghi
            homeCoords,      // Vị trí bắt đầu (0, 0)
            &count           // Biến nhận số lượng ký tự thực tế đã ghi
        )) return;

        // 3. (Tùy chọn) Đặt lại thuộc tính màu sắc (tránh việc màu nền cũ còn sót lại)
        if (!FillConsoleOutputAttribute(
            hConsole,
            csbi.wAttributes,
            cellCount,
            homeCoords,
            &count
        )) return;

        // 4. Đưa con trỏ về vị trí góc trên bên trái (0, 0)
        SetConsoleCursorPosition(hConsole, homeCoords);
    }
    void path(const vector<string> &args){
        // cout << "Current PATH:\n";
        // for(const auto& p : pathList) cout << p << "\n";
        char buffer[4096];
        DWORD size = GetEnvironmentVariableA("PATH", buffer, sizeof(buffer));
        if(size > 0 && size < sizeof(buffer)){
            cout << "Current PATH:\n" << string(buffer) << "\n";
        } else {
            cout << "Failed to get PATH variable.\n";
        }
    }
    void addPath(const vector<string> &args){
        if(args.empty()){
            cout << "Usage: addpath [new_path]\n";
            return;
        }
        string newPath = args[0];
        char buffer[4096];
        DWORD size = GetEnvironmentVariableA("PATH", buffer, sizeof(buffer));
        string newPathStr = (size > 0 && size < sizeof(buffer)) ? string(buffer) + ";" + newPath : newPath;
        if(SetEnvironmentVariableA("PATH", newPathStr.c_str())){
            cout << "Added to PATH: " << newPath << "\n";
        } else {
            cout << "Failed to add to PATH.\n";
        }
    }
    void showHistory(const vector<string> &args){
        cout << "Command History:\n";
        for(const auto& cmd : commandHistory) cout << cmd << "\n";
    }
    void runCommand(const Command& cmd){
        if (cmd.name == "help") help(cmd.args);
        else if (cmd.name == "exit") exit(cmd.args);
        else if (cmd.name == "cls") cls(cmd.args);
        else if (cmd.name == "path") path(cmd.args);
        else if (cmd.name == "addpath") addPath(cmd.args);
        else if (cmd.name == "history") showHistory(cmd.args);
        else cout << "Unknown command: " << cmd.name << "\n";
        commandHistory.push_back(cmd.name);
    }
    

public:
    void run(){
        while(true){
            cout << "~ ";
            string cmd;
            getline(cin, cmd);
            vector<string> tokens = split(cmd); 
            if(tokens.empty()) continue;
            for(auto &c: tokens[0]) c = tolower(c);
            cout << "- ";
            Command command{tokens[0], vector<string>(tokens.begin() + 1, tokens.end())};
            runCommand(command);
        }
    }
};

signed main(){
    TinyShell shell;
    shell.run();
}
