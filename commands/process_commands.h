#ifndef PROCESS_COMMANDS_H
#define PROCESS_COMMANDS_H

#include "../command_manager.h"
#include <windows.h>
#include <tlhelp32.h>
#include <sstream>
#include <signal.h>
#include <sys/types.h>
#include <vector>
#include <string>
#include "../utils.h"

// Struct lưu thông tin tiến trình background
struct Job {
    DWORD pid;
    HANDLE hProcess;
    std::string cmdLine;
};

// Struct quản lý tiến trình foreground
struct ForegroundProcess {
    DWORD pid;
    HANDLE hProcess;
    std::string command;
    bool isSuspended;
};

// Biến toàn cục lưu danh sách tiến trình nền
extern std::vector<Job> backgroundJobs;

ForegroundProcess currentForegroundProcess = {0, NULL, "", false};

BOOL SuspendForegroundProcess() {
    if (currentForegroundProcess.hProcess == NULL || currentForegroundProcess.isSuspended) {
        return FALSE;
    }

    // Tạm dừng tất cả thread của tiến trình
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te32;
        te32.dwSize = sizeof(THREADENTRY32);
        
        if (Thread32First(hThreadSnapshot, &te32)) {
            do {
                if (te32.th32OwnerProcessID == currentForegroundProcess.pid) {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                    if (hThread != NULL) {
                        SuspendThread(hThread);
                        CloseHandle(hThread);
                    }
                }
            } while (Thread32Next(hThreadSnapshot, &te32));
        }
        CloseHandle(hThreadSnapshot);
    }

    currentForegroundProcess.isSuspended = true;
    std::cout << "\n[Suspended] " << currentForegroundProcess.pid << " - " 
              << currentForegroundProcess.command << std::endl;
    
    return TRUE;
}

BOOL ResumeForegroundProcess() {
    if (currentForegroundProcess.hProcess == NULL || !currentForegroundProcess.isSuspended) {
        return FALSE;
    }

    // Tiếp tục tất cả thread của tiến trình
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te32;
        te32.dwSize = sizeof(THREADENTRY32);
        
        if (Thread32First(hThreadSnapshot, &te32)) {
            do {
                if (te32.th32OwnerProcessID == currentForegroundProcess.pid) {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                    if (hThread != NULL) {
                        ResumeThread(hThread);
                        CloseHandle(hThread);
                    }
                }
            } while (Thread32Next(hThreadSnapshot, &te32));
        }
        CloseHandle(hThreadSnapshot);
    }

    currentForegroundProcess.isSuspended = false;
    std::cout << "[Resumed] " << currentForegroundProcess.pid << " - " 
              << currentForegroundProcess.command << std::endl;
    
    return TRUE;
}

void fgCommand(const std::vector<std::string>& args) {
    if (currentForegroundProcess.hProcess == NULL) {
        std::cout << "No suspended foreground process" << std::endl;
        return;
    }

    if (!currentForegroundProcess.isSuspended) {
        std::cout << "No suspended process to resume" << std::endl;
        return;
    }

    if (ResumeForegroundProcess()) {
        // Đợi tiến trình hoàn thành
        WaitForSingleObject(currentForegroundProcess.hProcess, INFINITE);
        
        // Dọn dẹp
        CloseHandle(currentForegroundProcess.hProcess);
        currentForegroundProcess = {0, NULL, "", false};
    }
}

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
    cmdMutable.push_back('\0');

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    // QUAN TRỌNG: Thêm cờ DETACHED_PROCESS để tránh mở cửa sổ mới
    DWORD creationFlags = isBackground ? DETACHED_PROCESS : 0;
    
    // QUAN TRỌNG: Đặt CREATE_NO_WINDOW cho console apps
    if (cmdLine.find(".exe") != std::string::npos) {
        creationFlags |= CREATE_NO_WINDOW;
    }

    // Tạo process với CREATE_NEW_PROCESS_GROUP để kiểm soát tốt hơn
    if (!CreateProcessA(NULL, cmdMutable.data(), NULL, NULL, FALSE, 
                       CREATE_NEW_PROCESS_GROUP | creationFlags,
                       NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to start process. Error: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Started process with PID: " << pi.dwProcessId << std::endl;

    if (isBackground) {
        backgroundJobs.push_back({ pi.dwProcessId, pi.hProcess, cmdLine });
        CloseHandle(pi.hThread);
    } else {
        currentForegroundProcess.pid = pi.dwProcessId;
        currentForegroundProcess.hProcess = pi.hProcess;
        currentForegroundProcess.command = cmdLine;
        currentForegroundProcess.isSuspended = false;

        WaitForSingleObject(pi.hProcess, INFINITE);

        // // Đợi với timeout ngắn để kiểm tra trạng thái
        // while (WaitForSingleObject(pi.hProcess, 100) == WAIT_TIMEOUT) {
        //     if (currentForegroundProcess.isSuspended) {
        //         break; // Thoát nếu process bị suspend
        //     }
        // }

        // Chỉ đóng handle nếu process đã kết thúc
        if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        currentForegroundProcess = {0, NULL, "", false};
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

// Handler cho Ctrl+C và Ctrl+Break
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        if (currentForegroundProcess.hProcess != NULL && !currentForegroundProcess.isSuspended) {
            // Tạm dừng tiến trình
            HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
                THREADENTRY32 te32;
                te32.dwSize = sizeof(THREADENTRY32);
                
                if (Thread32First(hThreadSnapshot, &te32)) {
                    do {
                        if (te32.th32OwnerProcessID == currentForegroundProcess.pid) {
                            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                            if (hThread != NULL) {
                                SuspendThread(hThread);
                                CloseHandle(hThread);
                            }
                        }
                    } while (Thread32Next(hThreadSnapshot, &te32));
                }
                CloseHandle(hThreadSnapshot);
            }

            currentForegroundProcess.isSuspended = true;
            std::cout << "\n[Suspended] " << currentForegroundProcess.pid << " - " 
                      << currentForegroundProcess.command << std::endl;
            
            // QUAN TRỌNG: Đóng handle và reset tiến trình foreground
            CloseHandle(currentForegroundProcess.hProcess);
            currentForegroundProcess = {0, NULL, "", false};
            
            // Hiển thị lại prompt
            char currentDir[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDir);
            set_color(10);
            std::cout << currentDir << "> ";
            set_color(7);
            
            return TRUE; // Ngăn không cho thoát chương trình
        }
    }
    return FALSE;
}

void SetupConsoleCtrlHandler() {
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

// Thêm prototype cho hàm bgCommand
void bgCommand(const std::vector<std::string>& args);

// Hàm xử lý lệnh bg
void bgCommand(const std::vector<std::string>& args) {
    if (currentForegroundProcess.hProcess == NULL) {
        std::cout << "No suspended foreground process" << std::endl;
        return;
    }

    if (!currentForegroundProcess.isSuspended) {
        std::cout << "No suspended process to background" << std::endl;
        return;
    }

    // Tiếp tục tiến trình nhưng để chạy nền
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te32;
        te32.dwSize = sizeof(THREADENTRY32);
        
        if (Thread32First(hThreadSnapshot, &te32)) {
            do {
                if (te32.th32OwnerProcessID == currentForegroundProcess.pid) {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                    if (hThread != NULL) {
                        ResumeThread(hThread);
                        CloseHandle(hThread);
                    }
                }
            } while (Thread32Next(hThreadSnapshot, &te32));
        }
        CloseHandle(hThreadSnapshot);
    }

    // Thêm vào danh sách job background
    backgroundJobs.push_back({
        currentForegroundProcess.pid,
        currentForegroundProcess.hProcess,
        currentForegroundProcess.command
    });

    std::cout << "[Background] " << currentForegroundProcess.pid << " - " 
              << currentForegroundProcess.command << std::endl;

    // Reset thông tin foreground process
    currentForegroundProcess = {0, NULL, "", false};
}

#endif
