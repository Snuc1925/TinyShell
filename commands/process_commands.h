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

void findProcessByName(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: find <process_name>\n";
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

DWORD getPIDByName(const std::string& processName) {
    std::wstring targetName(processName.begin(), processName.end());

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    DWORD pid = 0;
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (targetName == pe.szExeFile) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return pid;
}

#endif
