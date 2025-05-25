#include "command_manager.h"
#include "commands/file_commands.h"
#include "commands/process_commands.h"
#include "utils.h"

CommandManager shell;
std::vector<Job> backgroundJobs;

void registerCommand()
{
    // ---- File Management -------------
    shell.registerCommand("ls", listFiles);        // Liệt kê các file và folder ở folder hiện tại
    shell.registerCommand("mkdir", makeDirectory); // Tạo folder mới tại folder hiện tại
    shell.registerCommand("cd", changeDirectory);

    // ---- Process Management ----------
    shell.registerCommand("ps", listProcesses);
    shell.registerCommand("run", runExternalCommand);
    shell.registerCommand("jobs", jobsCommand);
    shell.registerCommand("kill", killCommand);       // Kết thúc tiến trình theo PID: kill <PID>
    shell.registerCommand("killall", killAllCommand); // Kết thúc tất cả tiến trình có cùng tên: killall <process_name>

    // ---- File Utility -------------
    shell.registerCommand("touch", touchCommand); // Create an empty file: touch file
    shell.registerCommand("cat", catCommand);     // View contents of a file: cat file
    shell.registerCommand("head", headCommand);   // Display the first 10 lines of file: head file
}

int main()
{
    char currentDir[MAX_PATH];

    registerCommand();

    std::string input;
    while (true)
    {
        GetCurrentDirectory(MAX_PATH, currentDir);

        set_color(10); // Màu xanh lá cho prompt
        std::cout << currentDir << "> ";
        set_color(7); // Màu trắng cho input

        std::getline(std::cin, input);
        if (input == "exit")
            break;
        shell.executeCommand(input);
    }

    return 0;
}
