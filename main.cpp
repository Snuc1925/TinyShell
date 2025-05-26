#include "command_manager.h"
#include "commands/file_commands.h"
#include "commands/process_commands.h"
#include "utils.h"
#include "commands/disk_commands.h"
#include <windows.h>

CommandManager shell;
std::vector<Job> backgroundJobs;

void registerCommand() {
    // ---- File Management -------------
    shell.registerCommand("ls", listFiles); // Liệt kê các file và folder ở folder hiện tại
    shell.registerCommand("mkdir", makeDirectory); // Tạo folder mới tại folder hiện tại 
    shell.registerCommand("cd", changeDirectory);

    // ---- Process Management ----------
    shell.registerCommand("ps", listProcesses); 
    shell.registerCommand("run", runExternalCommand);
    shell.registerCommand("jobs", jobsCommand);
    shell.registerCommand("top", topCommand);
    shell.registerCommand("fg", fgCommand);
    shell.registerCommand("fg %id", fgIdCommand);
    
    // ---- Disk Management -----------
    shell.registerCommand("df", checkCapacityMemory); // Kiểm tra dung lượng ổ đĩa
    shell.registerCommand("du", showCapacityFolder); // Kiểm tra dung lượng folder
    shell.registerCommand("di", showCapacityFile); // Kiểm tra dung lượng file
    shell.registerCommand("clear", clearCLS); //Lệnh clear tương tự như lệnh cls trong Windows
}

int main() {
    char currentDir[MAX_PATH];

    registerCommand();

    std::string input;
    while (true) {
        GetCurrentDirectoryA(MAX_PATH, currentDir);

        set_color(10); // Màu xanh lá cho prompt
        std::cout << currentDir << "> ";
        set_color(7);  // Màu trắng cho input

        std::getline(std::cin, input);
        if (input == "exit") break;
        shell.executeCommand(input);
    }

    return 0;
}

