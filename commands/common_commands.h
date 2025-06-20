#ifndef COMMON_COMMANDS_H
#define COMMON_COMMANDS_H

#include "../command_manager.h"
#include "../commands/process_commands.h"
#include <windows.h>
#include <tlhelp32.h>
#include <sstream>
#include <algorithm>  
#include <vector>

// void exitCommand(const std::vector<std::string>& args) {
//     std::cout << "Đang thoát shell...\n";
//     std::exit(0);

//     // exit -r : thoát và dừng hết tiến trình do tinyshell quản lý
//     // exit : thoát và vẫn giữ nguyên tiến trình
// }

// Hàm hỗ trợ để kết thúc tất cả các tiến trình con
void terminateManagedProcesses() {
    if (processList.empty()) {
        std::cout << "No managed processes to terminate.\n";
        return;
    }

    std::cout << "Terminating all managed processes...\n";
    
    // Iterate through all processes in processList
    for (auto it = processList.begin(); it != processList.end(); ) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, it->pid);
        if (hProcess) {
            if (TerminateProcess(hProcess, 0)) {
                std::cout << "Terminated PID: " << it->pid << " - " << it->cmdLine << "\n";
                CloseHandle(hProcess);
                CloseHandle(it->hProcess);  // Close the stored handle too
                it = processList.erase(it);
                continue;  // Skip increment since we erased
            }
            CloseHandle(hProcess);
        } else {
            std::cerr << "Failed to open process PID: " << it->pid << "\n";
        }
        ++it;
    }
    
    std::cout << "All managed processes terminated.\n";
}

void exitCommand(const std::vector<std::string>& args) {
    if (args.size() > 0 && args[0] == "-r") {
        std::cout << "Stopping all processes and exit the shell...\n";
        terminateManagedProcesses();
    } else {
        std::cout << "Exit the shell...\n";
    }
    
    std::exit(0);
}

void helpCommand(const std::vector<std::string>& args) {
    std::cout << "==== List of Supported Commands ====\n\n";

    std::cout << "-- File Management --\n";
    std::cout << "  ls               : List files and folders in the current directory\n";
    std::cout << "  mkdir <folder>   : Create a new folder\n";
    std::cout << "  cd <path>        : Change current directory\n";
    std::cout << "  pwd              : Print the current working directory\n";
    std::cout << "  rm <file/folder> : Delete a file or folder (use -r for recursive)\n";
    std::cout << "  cp <src> <dst>   : Copy a file or folder\n";
    std::cout << "  mv <src> <dst>   : Move or rename a file or folder\n";
    std::cout << "  touch <file>     : Create a new empty file\n";
    std::cout << "  cat <file>       : Display the contents of a file\n";
    std::cout << "  head <file>      : Show the first 10 lines of a file\n\n";

    std::cout << "-- Process Management --\n";
    std::cout << "  run <cmd> [&]    : Run a program (use & to run in background)\n";
    std::cout << "  ps               : List running processes\n";
    std::cout << "  top              : Show real-time process usage\n";
    std::cout << "  pgrep <name>     : Find a process by name\n";
    std::cout << "  suspend <PID>    : Suspend a process\n";
    std::cout << "  resume <PID>     : Resume a suspended process\n";
    std::cout << "  fg <PID>         : Bring a background process to foreground\n";
    std::cout << "  kill <PID>       : Terminate a process by PID\n";
    std::cout << "  killall <name>   : Terminate all processes with the given name\n\n";

    std::cout << "-- Disk Management --\n";
    std::cout << "  df               : Check disk usage\n";
    std::cout << "  du <folder>      : Show disk usage of a folder\n";
    std::cout << "  di <file>        : Show disk usage of a file\n\n";

    std::cout << "-- Common --\n";
    std::cout << "  help             : Display this help menu\n";
    std::cout << "  exit             : Exit the shell\n";
    std::cout << "  clear            : Clear the screen (similar to 'cls')\n";

    std::cout << "\n====================================\n";
}


void clearScreenCommand(const std::vector<std::string>& args) {
    // Like the 'cls' command in Windows, this function clears the console screen.
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD written;
    if (hConsole == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to get console handle." << std::endl;
        return;
    }
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        std::cerr << "Error: Unable to get console buffer info." << std::endl;
        return;
    }
    DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
    if (!FillConsoleOutputCharacterA(hConsole, ' ', size, coord, &written)) {
        std::cerr << "Error: Unable to fill console output." << std::endl;
        return;
    }
    if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, size, coord, &written)) {
        std::cerr << "Error: Unable to fill console attributes." << std::endl;
        return;
    }
    SetConsoleCursorPosition(hConsole, coord);
}

#endif