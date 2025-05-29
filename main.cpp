#include "command_manager.h"
#include "commands/file_commands.h"
#include "commands/process_commands.h"
#include "commands/disk_commands.h"
#include "utils.h"

CommandManager shell;
// Global variables
std::vector<Job> backgroundJobs;
DWORD currentForegroundPid = 0;
HANDLE currentForegroundProcess = NULL;

void registerCommand()
{
    // ---- File Management -------------
    shell.registerCommand("ls", listFiles);        // Liệt kê các file và folder ở folder hiện tại
    shell.registerCommand("mkdir", makeDirectory); // Tạo folder mới tại folder hiện tại
    shell.registerCommand("cd", changeDirectory);
    shell.registerCommand("pwd", printCurrentDirectory); // Hiển thị đường dẫn hiện tại
    shell.registerCommand("rm", removeCommand); // Xóa file hoặc folder: rm file, rm -r folder
    shell.registerCommand("cp", copyFile);
    shell.registerCommand("mv", moveFileOrDirectory);
    shell.registerCommand("touch", touchCommand); // Create an empty file: touch file
    shell.registerCommand("cat", catCommand);     // View contents of a file: cat file
    shell.registerCommand("head", headCommand);   // Display the first 10 lines of file: head file

    // ---- Process Management ----------
    shell.registerCommand("run", runExternalCommand);    // Chạy tiến trình: run python main.py (fg); run python main.py & (bg)
    shell.registerCommand("ps", listProcessCommand);     // Hiển thị danh sách tiến trình đang chạy 
    shell.registerCommand("top", topCommand);            // !!! [Cái này chưa ổn] Hiển thị tiến trình theo thời gian thực (background mode) 
    shell.registerCommand("pgrep", findProcessByName);   // Tìm kiếm tiến trình theo tên
    shell.registerCommand("suspend", suspendCommand);    // Tạm dừng tiến trình: suspend <PID>
    shell.registerCommand("resume", resumeCommand);      // Resume một tiến trình đã bị tạm dừng ở dạng background: resume <PID>
    shell.registerCommand("fg", fgCommand);              // Thêm lệnh fg <pid> để đưa tiến trình từ background lên foreground
    shell.registerCommand("kill", killCommand);          // Kết thúc tiến trình theo PID: kill <PID>
    shell.registerCommand("killall", killAllCommand);    // Kết thúc tất cả tiến trình có cùng tên: killall <process_name>
    // ----  

    // ---- Disk Management -----------
    shell.registerCommand("df", checkCapacityMemory); // Kiểm tra dung lượng ổ đĩa
    shell.registerCommand("du", showCapacityFolder); // Kiểm tra dung lượng folder
    shell.registerCommand("di", showCapacityFile); // Kiểm tra dung lượng file
    shell.registerCommand("clear", clearCLS); //Lệnh clear tương tự như lệnh cls trong Windows

    // Thiết lập handler
    SetupConsoleCtrlHandler();
}

int main()
{
    char currentDir[MAX_PATH];
    SetupConsoleCtrlHandler();
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
