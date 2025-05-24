// commands/file_commands.h
#ifndef FILE_COMMANDS_H
#define FILE_COMMANDS_H

#include "../command_manager.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

void listFiles(const std::vector<std::string>& args) {
    std::string path = (args.empty()) ? "." : args[0];
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to open directory." << std::endl;
        return;
    }

    do {
        std::string fileName = findFileData.cFileName;
        if (fileName != "." && fileName != "..") {  // Bỏ qua "." và ".."
            std::cout << fileName << std::endl;
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);
    
    FindClose(hFind);
}

void makeDirectory(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: Missing directory name." << std::endl;
        return;
    }

    std::string dirName = args[0];
    if (CreateDirectoryA(dirName.c_str(), NULL)) {
        std::cout << "Directory created: " << dirName << std::endl;
    } else {
        std::cerr << "Error: Unable to create directory." << std::endl;
    }
}

void changeDirectory(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: Missing directory argument." << std::endl;
        return;
    }

    if (!SetCurrentDirectoryA(args[0].c_str())) {
        std::cerr << "Error: Unable to change directory to " << args[0] << std::endl;
    }
}


// Hàm copy đệ quy thư mục (Windows API)
bool copyDirectoryRecursive(const std::string& src, const std::string& dst) {
    WIN32_FIND_DATAA findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    
    std::string searchPath = src + "\\*";
    hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Cannot open directory " << src << std::endl;
        return false;
    }

    // Tạo thư mục đích nếu chưa tồn tại
    CreateDirectoryA(dst.c_str(), NULL);

    do {
        const std::string fileOrDirName = findData.cFileName;
        
        // Bỏ qua . và ..
        if (fileOrDirName == "." || fileOrDirName == "..") {
            continue;
        }

        std::string srcPath = src + "\\" + fileOrDirName;
        std::string dstPath = dst + "\\" + fileOrDirName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Nếu là thư mục, copy đệ quy
            if (!copyDirectoryRecursive(srcPath, dstPath)) {
                FindClose(hFind);
                return false;
            }
        } else {
            // Nếu là file, copy file
            if (!CopyFileA(srcPath.c_str(), dstPath.c_str(), FALSE)) {
                std::cerr << "Error copying file " << srcPath << " to " << dstPath << std::endl;
                FindClose(hFind);
                return false;
            }
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
    return true;
}

// Hàm xử lý lệnh cp với option -r
void copyFile(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: cp [-r] <source> <destination>" << std::endl;
        return;
    }

    bool recursive = false;
    size_t src_index = 0;

    // Kiểm tra option -r
    if (args[0] == "-r") {
        if (args.size() < 3) {
            std::cerr << "Usage: cp -r <source> <destination>" << std::endl;
            return;
        }
        recursive = true;
        src_index = 1;
    }

    const std::string& source = args[src_index];
    const std::string& dest = args[src_index + 1];

    if (recursive) {
        // Xử lý copy đệ quy thư mục
        DWORD attr = GetFileAttributesA(source.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            std::cerr << "Error: Source is not a directory" << std::endl;
            return;
        }

        if (copyDirectoryRecursive(source, dest)) {
            std::cout << "Successfully copied directory '" << source << "' to '" << dest << "'" << std::endl;
        }
    } else {
        // Xử lý copy file thông thường
        DWORD attr = GetFileAttributesA(source.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            std::cerr << "Error: Source is a directory (use -r for directories)" << std::endl;
            return;
        }

        if (CopyFileA(source.c_str(), dest.c_str(), FALSE)) {
            std::cout << "Successfully copied '" << source << "' to '" << dest << "'" << std::endl;
        } else {
            std::cerr << "Error copying file" << std::endl;
        }
    }
}



#endif
