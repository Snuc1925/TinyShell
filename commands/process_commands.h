#ifndef PROCESS_COMMANDS_H
#define PROCESS_COMMANDS_H

#include "../command_manager.h"
#include <windows.h>
#include <tlhelp32.h>
#include <sstream>
#include <algorithm>  // Added for std::remove_if
#include <vector>

// Struct lưu thông tin tiến trình background
struct Job
{
    DWORD pid;
    HANDLE hProcess;
    std::string cmdLine;
    bool isSuspended;
};

// Forward declarations
BOOL SuspendProcess(DWORD pid);
BOOL ResumeProcess(DWORD pid);
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType);

// Biến toàn cục
extern std::vector<Job> backgroundJobs;
extern DWORD currentForegroundPid;
extern HANDLE currentForegroundProcess;

void SetupConsoleCtrlHandler() {
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

void listProcesses(const std::vector<std::string> &args)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error: Unable to create process snapshot." << std::endl;
        return;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe))
    {
        do
        {
            std::cout << "PID: " << pe.th32ProcessID << " - " << pe.szExeFile << std::endl;
        } while (Process32Next(hSnap, &pe));
    }
    else
    {
        std::cerr << "Error: Unable to retrieve process list." << std::endl;
    }

    CloseHandle(hSnap);
}

void findProcessByName(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: pgrep <process_name>\n";
        return;
    }

    // Chuyển std::string sang std::wstring
    std::wstring targetName(args[0].begin(), args[0].end());

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to create process snapshot.\n";
        return;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    bool found = false;
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (targetName == pe.szExeFile) {
                std::wcout << L"Found: PID " << pe.th32ProcessID << L" - " << pe.szExeFile << std::endl;
                found = true;
            }
        } while (Process32NextW(hSnap, &pe));
    }

    if (!found) {
        std::wcout << L"No process found with name: " << targetName << std::endl;
    }

    CloseHandle(hSnap);
}



void runExternalCommand(const std::vector<std::string> &args)
{
    bool isBackground = false;
    std::vector<std::string> modifiableArgs = args;

    if (!modifiableArgs.empty() && modifiableArgs.back() == "&")
    {
        isBackground = true;
        modifiableArgs.pop_back();
    }

    std::ostringstream oss;
    for (size_t i = 0; i < modifiableArgs.size(); ++i)
    {
        if (i > 0)
            oss << " ";
        oss << modifiableArgs[i];
    }
    std::string cmdLine = oss.str();

    std::vector<char> cmdMutable(cmdLine.begin(), cmdLine.end());
    cmdMutable.push_back('\0'); // ensure null-terminated

    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;

    DWORD creationFlags = isBackground ? CREATE_NEW_CONSOLE : 0;

    BOOL success = CreateProcessA(
        NULL,
        cmdMutable.data(),
        NULL, NULL, FALSE,
        creationFlags,
        NULL, NULL,
        &si,
        &pi);

    if (!success)
    {
        std::cerr << "Failed to start process.\n";
        return;
    }

    std::cout << "Started process with PID: " << pi.dwProcessId << "\n";

    if (isBackground) {
        backgroundJobs.push_back({ pi.dwProcessId, pi.hProcess, cmdLine, false });
        CloseHandle(pi.hThread);
    } else {
        currentForegroundPid = pi.dwProcessId;
        currentForegroundProcess = pi.hProcess;
        
        // Đợi với timeout thay vì INFINITE
        while (WaitForSingleObject(pi.hProcess, 100) == WAIT_TIMEOUT) {
            if (currentForegroundPid == 0) {  // Đã bị tạm dừng
                backgroundJobs.push_back({ pi.dwProcessId, pi.hProcess, cmdLine, true });
                return;
            }
        }
        
        // Nếu tiến trình kết thúc bình thường
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        currentForegroundPid = 0;
        currentForegroundProcess = NULL;
    }
}

void jobsCommand(const std::vector<std::string> &args)
{
    // Cập nhật danh sách jobs: loại bỏ tiến trình đã kết thúc
    for (auto it = backgroundJobs.begin(); it != backgroundJobs.end();)
    {
        DWORD status = WaitForSingleObject(it->hProcess, 0);
        if (status == WAIT_OBJECT_0)
        {
            // tiến trình đã kết thúc
            CloseHandle(it->hProcess);
            it = backgroundJobs.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (backgroundJobs.empty())
    {
        std::cout << "No background jobs.\n";
        return;
    }

    std::cout << "Background jobs:\n";
    for (const auto& job : backgroundJobs) {
        std::cout << "PID: " << job.pid << " - " << job.cmdLine 
                  << " [" << (job.isSuspended ? "Suspended" : "Running") << "]\n";
    }
}

BOOL SuspendProcess(DWORD pid) {
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);
    
    if (Thread32First(hThreadSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == pid) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                if (hThread != NULL) {
                    SuspendThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hThreadSnapshot, &te32));
    }
    
    CloseHandle(hThreadSnapshot);
    return TRUE;
}

BOOL ResumeProcess(DWORD pid) {
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);
    
    if (Thread32First(hThreadSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == pid) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                if (hThread != NULL) {
                    ResumeThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hThreadSnapshot, &te32));
    }
    
    CloseHandle(hThreadSnapshot);
    return TRUE;
}

// BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
//     if (dwCtrlType == CTRL_C_EVENT && currentForegroundPid != 0) {
//         if (SuspendProcess(currentForegroundPid)) {
//             backgroundJobs.push_back({ currentForegroundPid, currentForegroundProcess, "", true });
//             std::cout << "\n[Suspended] " << currentForegroundPid << std::endl;
//             currentForegroundPid = 0;
//             currentForegroundProcess = NULL;
//             return TRUE;
//         }
//     }
//     return FALSE;
// }

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT && currentForegroundPid != 0) {
        if (SuspendProcess(currentForegroundPid)) {
            backgroundJobs.push_back({ currentForegroundPid, currentForegroundProcess, "", true });
            std::cout << "\n[Suspended] " << currentForegroundPid << std::endl;
            
            // Reset foreground process
            currentForegroundPid = 0;
            currentForegroundProcess = NULL;
            
            // QUAN TRỌNG: Hiển thị lại prompt
            char currentDir[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDir);
            
            return TRUE;  // Ngăn không cho shell thoát
        }
    }
    return FALSE;
}

void topCommand(const std::vector<std::string>& args) {
    //Hiển thị tiến trình theo thời gian thực. (background mode)
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to create process snapshot." << std::endl;
        return;
    }
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnap, &pe)) {
        std::cout << "PID\tProcess Name\n";
        do {
            std::cout << pe.th32ProcessID << "\t" << pe.szExeFile << std::endl;
        } while (Process32Next(hSnap, &pe));
    } else {
        std::cerr << "Error: Unable to retrieve process list." << std::endl;
    }
    CloseHandle(hSnap);
    std::cout << "Press Ctrl+C to stop.\n";
    // Giữ cho tiến trình chạy liên tục
    while (true) {
        Sleep(1000); // Cập nhật mỗi giây
        system("cls"); // Xóa
        hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) {
            std::cerr << "Error: Unable to create process snapshot." << std::endl;
            return;
        }
        if (Process32First(hSnap, &pe)) {
            std::cout << "PID\tProcess Name\n";
            do {
                std::cout << pe.th32ProcessID << "\t" << pe.szExeFile << std::endl;
            } while (Process32Next(hSnap, &pe));
        } else {
            std::cerr << "Error: Unable to retrieve process list." << std::endl;
        }
    }

        CloseHandle(hSnap);
        std::cout << "Press Ctrl+C to stop.\n";
        // Cập nhật danh sách tiến trình nền
        for (auto it = backgroundJobs.begin(); it != backgroundJobs.end();) {
            DWORD status = WaitForSingleObject(it->hProcess, 0);
            if (status == WAIT_OBJECT_0) {
                // tiến trình đã kết thúc
                CloseHandle(it->hProcess);
                it = backgroundJobs.erase(it);
            } else {
                ++it;
            }
        }
        if (backgroundJobs.empty()) {
            std::cout << "No background jobs.\n";
        } else {
            std::cout << "Background jobs:\n";
            for (const auto& job : backgroundJobs) {
                std::cout << "PID: " << job.pid << " - " << job.cmdLine << std::endl;
            }
        }

        std::cout << "Press Ctrl+C to stop.\n";

        // Đợi 1 giây trước khi cập nhật lại
        Sleep(1000);

        //Kiêm tra xem có tiến trình nào đã kết thúc không
        for (auto it = backgroundJobs.begin(); it != backgroundJobs.end();) {
            DWORD status = WaitForSingleObject(it->hProcess, 0);
            if (status == WAIT_OBJECT_0) {
                // tiến trình đã kết thúc
                CloseHandle(it->hProcess);
                it = backgroundJobs.erase(it);
            } else {
                ++it;
            }
        }

        if (backgroundJobs.empty()) {
            std::cout << "No background jobs.\n";
        } else {
            std::cout << "Background jobs:\n";
            for (const auto& job : backgroundJobs) {
                std::cout << "PID: " << job.pid << " - " << job.cmdLine << std::endl;
            }
        }


        std::cout << "Press Ctrl+C to stop.\n";
    
}


void fgCommand(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: fg <pid>" << std::endl;
        return;
    }

    DWORD pid;
    try {
        pid = std::stoul(args[0]);
    } catch (...) {
        std::cerr << "Invalid PID" << std::endl;
        return;
    }

    for (auto it = backgroundJobs.begin(); it != backgroundJobs.end(); ++it) {
        if (it->pid == pid && it->isSuspended) {
            if (ResumeProcess(pid)) {
                currentForegroundPid = pid;
                currentForegroundProcess = it->hProcess;
                WaitForSingleObject(it->hProcess, INFINITE);
                CloseHandle(it->hProcess);
                backgroundJobs.erase(it);
                currentForegroundPid = 0;
                currentForegroundProcess = NULL;
                return;
            }
        }
    }
    std::cerr << "No suspended process with PID " << pid << std::endl;
}

void bgCommand(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: bg <pid>" << std::endl;
        return;
    }

    DWORD pid;
    try {
        pid = std::stoul(args[0]);
    } catch (...) {
        std::cerr << "Invalid PID" << std::endl;
        return;
    }

    for (auto& job : backgroundJobs) {
        if (job.pid == pid && job.isSuspended) {
            if (ResumeProcess(pid)) {
                job.isSuspended = false;
                std::cout << "[Resumed in background] " << pid << std::endl;
                return;
            }
        }
    }
    std::cerr << "No suspended process with PID " << pid << std::endl;
}

void suspendCommand(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: suspend <pid>" << std::endl;
        return;
    }

    DWORD pid;
    try {
        pid = std::stoul(args[0]);
    } catch (...) {
        std::cerr << "Invalid PID" << std::endl;
        return;
    }

    for (auto& job : backgroundJobs) {
        if (job.pid == pid && !job.isSuspended) {
            if (SuspendProcess(pid)) {
                job.isSuspended = true;
                std::cout << "[Suspended] " << pid << std::endl;
                return;
            }
        }
    }
    std::cerr << "No running process with PID " << pid << std::endl;
}

void killCommand(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        std::cerr << "Error: Missing PID argument." << std::endl;
        return;
    }
    DWORD pid = 0;
    try
    {
        pid = std::stoul(args[0]);
    }
    catch (...)
    {
        std::cerr << "Error: Invalid PID." << std::endl;
        return;
    }
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess)
    {
        std::cerr << "Error: Unable to open process with PID " << pid << std::endl;
        return;
    }
    if (!TerminateProcess(hProcess, 0))
    {
        std::cerr << "Error: Unable to terminate process." << std::endl;
    }
    else
    {
        std::cout << "Process " << pid << " terminated." << std::endl;
    }
    CloseHandle(hProcess);
}

void killAllCommand(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        std::cerr << "Error: Missing process name argument." << std::endl;
        return;
    }
    std::string targetName = args[0];
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error: Unable to create process snapshot." << std::endl;
        return;
    }
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    int killed = 0;
    if (Process32First(hSnap, &pe))
    {
        do
        {
            if (targetName == pe.szExeFile)
            {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess)
                {
                    if (TerminateProcess(hProcess, 0))
                    {
                        std::cout << "Terminated PID: " << pe.th32ProcessID << std::endl;
                        ++killed;
                    }
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
    if (killed == 0)
        std::cout << "No process named '" << targetName << "' found." << std::endl;
}

#endif