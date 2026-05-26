#include "../include/builtin.h"
#include "../include/process_mgr.h"
#include <iostream>
#include <windows.h>

bool HandleBuiltin(const std::vector<std::string>& args) {
    if (args.empty()) return false;

    // 1. Lệnh exit / quit
    if (args[0] == "exit" || args[0] == "quit") {
        ExitProcess(0);
    } 
    
    // 2. Lệnh trợ giúp help
    else if (args[0] == "help") {
        std::cout << "==================================================\n";
        std::cout << "TinyShell - Cac lenh duoc ho tro:\n";
        std::cout << "  help               : Hien thi huong dan nay\n";
        std::cout << "  exit / quit        : Thoat khoi Shell\n";
        std::cout << "  cls                : Xoa sach man hinh console\n";
        std::cout << "  cd <path>          : Thay doi thu muc (dung duoc ca 'cd ..')\n";
        std::cout << "  dir                : Liet ke file va thu muc hien tai\n";
        std::cout << "  date               : Xem ngay hien tai\n";
        std::cout << "  time               : Xem gio hien tai\n";
        std::cout << "  path               : Xem bien moi truong PATH\n";
        std::cout << "  addpath <path>     : Them mot duong dan vao bien PATH\n";
        std::cout << "  list               : Xem danh sach cac tien trinh background\n";
        std::cout << "  kill <PID>         : Ep buoc dung tien trinh background\n";
        std::cout << "  stop <PID>         : Tam dung tien trinh background\n";
        std::cout << "  resume <PID>       : Cho tien trinh tam dung chay tiep\n";
        std::cout << "==================================================\n";
        return true;
    }
    
    // 3. Lệnh cls (Clean Screen)
    else if (args[0] == "cls") {
        system("cls"); 
        return true;
    }
    
    // 4. Lệnh cd và cd .. (Change Directory)
    else if (args[0] == "cd") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Thieu duong dan cho lenh \"cd\"\n";
        } else {
            // SetCurrentDirectoryA tu dong hieu "." , ".." va cac duong dan tuyet doi/tuong doi
            if (!SetCurrentDirectoryA(args[1].c_str())) {
                std::cerr << "TinyShell: Khong the chuyen den thu muc nay. Ma loi: " << GetLastError() << "\n";
            }
        }
        return true;
    }
    
    // 5. Lệnh dir
    else if (args[0] == "dir") {
        system("dir"); 
        return true;
    }
    
    // 6. Lệnh date & time
    else if (args[0] == "date" || args[0] == "time") {
        SYSTEMTIME st;
        GetLocalTime(&st);
        if (args[0] == "date") 
            std::cout << "Ngay hien tai: " << st.wDay << "/" << st.wMonth << "/" << st.wYear << "\n";
        else 
            std::cout << "Gio hien tai: " << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "\n";
        return true;
    }
    
    // 7. Lệnh xem biến môi trường path
    else if (args[0] == "path") {
        char buffer[32767];
        if (GetEnvironmentVariableA("PATH", buffer, 32767)) {
            std::cout << "PATH=" << buffer << "\n";
        } else {
            std::cerr << "TinyShell: Khong the doc bien moi truong PATH. Ma loi: " << GetLastError() << "\n";
        }
        return true;
    }
    
    // 8. Lệnh thiết lập/thêm biến môi trường addpath
    else if (args[0] == "addpath") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Thieu tham so duong dan cho \"addpath\"\n";
        } else {
            char buffer[32767];
            GetEnvironmentVariableA("PATH", buffer, 32767);
            std::string newPath = std::string(buffer) + ";" + args[1];
            if (SetEnvironmentVariableA("PATH", newPath.c_str())) {
                std::cout << "TinyShell: Da them duong dan vao PATH thong cong.\n";
            } else {
                std::cerr << "TinyShell: Thay doi PATH that bai. Ma loi: " << GetLastError() << "\n";
            }
        }
        return true;
    }
    
    // 9. Lệnh quản lý danh sách tiến trình ngầm (list)
    else if (args[0] == "list") {
        ListProcesses();
        return true;
    }
    
    // 10. Lệnh kill <PID>
    else if (args[0] == "kill") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Thieu PID de kill.\n";
        } else {
            KillProcess(std::stoul(args[1]));
        }
        return true;
    }
    
    // 11. Lệnh stop <PID> (Suspend)
    else if (args[0] == "stop") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Thieu PID de dung (stop).\n";
        } else {
            StopProcess(std::stoul(args[1]));
        }
        return true;
    }
    
    // 12. Lệnh resume <PID> (Tiếp tục)
    else if (args[0] == "resume") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Thieu PID de tiep tuc (resume).\n";
        } else {
            ResumeProcess(std::stoul(args[1]));
        }
        return true;
    }

    return false; // Tra ve false neu khong phai lenh noi tru (de chuyen tiep sang external process)
}