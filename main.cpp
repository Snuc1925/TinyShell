#include "command_manager.h"
#include "commands/file_commands.h"
#include "commands/process_commands.h"
#include "utils.h"

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
    // ----  
}

int main() {
    char currentDir[MAX_PATH];

    registerCommand();

    std::string input;
    while (true) {
        GetCurrentDirectory(MAX_PATH, currentDir);

        set_color(10); // Màu xanh lá cho prompt
        std::cout << currentDir << "> ";
        set_color(7);  // Màu trắng cho input

        std::getline(std::cin, input);
        if (input == "exit") break;
        shell.executeCommand(input);
    }

    return 0;
}

