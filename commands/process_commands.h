#ifndef PROCESS_COMMANDS_H
#define PROCESS_COMMANDS_H

#include "../command_manager.h"
#include <windows.h>
#include <tlhelp32.h>
#include <sstream>

// Struct lưu thông tin tiến trình background
struct Job {
    DWORD pid;
    HANDLE hProcess;
    std::string cmdLine;
};

// Biến toàn cục lưu danh sách tiến trình nền
extern std::vector<Job> backgroundJobs;

void listProcesses(const std::vector<std::string>& args) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to create process snapshot." << std::endl;
        return;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe)) {
        do {
            std::cout << "PID: " << pe.th32ProcessID << " - " << pe.szExeFile << std::endl;
        } while (Process32Next(hSnap, &pe));
    } else {
        std::cerr << "Error: Unable to retrieve process list." << std::endl;
    }

    CloseHandle(hSnap);
}

void runExternalCommand(const std::vector<std::string>& args) {
    bool isBackground = false;
    std::vector<std::string> modifiableArgs = args;

    if (!modifiableArgs.empty() && modifiableArgs.back() == "&") {
        isBackground = true;
        modifiableArgs.pop_back();
    }

    std::ostringstream oss;
    for (size_t i = 0; i < modifiableArgs.size(); ++i) {
        if (i > 0) oss << " ";
        oss << modifiableArgs[i];
    }
    std::string cmdLine = oss.str();

    std::vector<char> cmdMutable(cmdLine.begin(), cmdLine.end());
    cmdMutable.push_back('\0');  // ensure null-terminated

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    DWORD creationFlags = isBackground ? CREATE_NEW_CONSOLE : 0;

    BOOL success = CreateProcessA(
        NULL,
        cmdMutable.data(),   
        NULL, NULL, FALSE,
        creationFlags,
        NULL, NULL,
        &si,
        &pi
    );

    if (!success) {
        std::cerr << "Failed to start process.\n";
        return;
    }

    std::cout << "Started process with PID: " << pi.dwProcessId << "\n";

    if (isBackground) {
        // Lưu tiến trình nền vào danh sách jobs
        backgroundJobs.push_back({ pi.dwProcessId, pi.hProcess, cmdLine });
        // Đóng thread handle nhưng giữ process handle để kiểm tra trạng thái
        CloseHandle(pi.hThread);
    } else {
        // Foreground: đợi tiến trình kết thúc rồi đóng handle
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

}

void jobsCommand(const std::vector<std::string>& args) {
    // Cập nhật danh sách jobs: loại bỏ tiến trình đã kết thúc
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
        return;
    }

    std::cout << "Background jobs:\n";
    for (const auto& job : backgroundJobs) {
        std::cout << "PID: " << job.pid << " - " << job.cmdLine << std::endl;
    }
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
    //Đưa job từ background lên foreground. Nếu có nhiều job thì lấy job thứ id
    if (args.size() < 2) {
        std::cerr << "Usage: fg <job_id>\n";
        return;
    }
    int jobId = std::stoi(args[1]);
    if (jobId < 0 || jobId >= backgroundJobs.size()) {
        std::cerr << "Invalid job ID.\n";
        return;
    }
    Job& job = backgroundJobs[jobId];
    // Đưa tiến trình lên foreground
    if (AttachConsole(job.pid)) {
        std::cout << "Attached to job with PID: " << job.pid << "\n";
        // Đợi tiến trình kết thúc
        WaitForSingleObject(job.hProcess, INFINITE);
        CloseHandle(job.hProcess);
        backgroundJobs.erase(backgroundJobs.begin() + jobId);
    } else {
        std::cerr << "Failed to attach to job.\n";
    }
}

void fgIdCommand(const std::vector<std::string>& args) {
    // Đưa job từ background lên foreground theo ID
    if (args.size() < 2) {
        std::cerr << "Usage: fg %<job_id>\n";
        return;
    }
    int jobId = std::stoi(args[1].substr(1)); // Bỏ ký tự '%'
    if (jobId < 0 || jobId >= backgroundJobs.size()) {
        std::cerr << "Invalid job ID.\n";
        return;
    }
    Job& job = backgroundJobs[jobId];
    // Đưa tiến trình lên foreground
    if (AttachConsole(job.pid)) {
        std::cout << "Attached to job with PID: " << job.pid << "\n";
        // Đợi tiến trình kết thúc
        WaitForSingleObject(job.hProcess, INFINITE);
        CloseHandle(job.hProcess);
        backgroundJobs.erase(backgroundJobs.begin() + jobId);
    } else {
        std::cerr << "Failed to attach to job.\n";
    }
}

#endif
