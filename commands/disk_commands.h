#ifndef DISK__COMMANDS_H
#define DISK__COMMANDS_H

#include "../command_manager.h"
#include <windows.h>
#include <iostream>
#include <string>

void checkCapacityMemory(const std::vector<std::string>& args){
    // Check if the user provided a drive letter
    if (args.empty()) {
        std::cerr << "Error: Missing drive letter." << std::endl;
        return;
    }
    
    std::string driveLetter = args[0];
    // Construct the drive path (e.g., "C:\\")
    std::string drivePath = driveLetter + ":\\";
    UINT driveType = GetDriveTypeA(drivePath.c_str());
    
    if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE) {
        // Get the free space on the drive
        ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
        if (GetDiskFreeSpaceExA(drivePath.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
            std::cout << "Drive " << driveLetter << ": " << std::endl;
            std::cout << "Total Space: " << totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024) << " GB" << std::endl;
            std::cout << "Free Space: " << freeBytesAvailable.QuadPart / (1024 * 1024 * 1024) << " GB" << std::endl;
        } else {
            std::cerr << "Error: Unable to get disk space information." << std::endl;
        }
    } else {
        std::cerr << "Error: Invalid drive letter or drive type." << std::endl;
    }
}

void showCapacityFolder(const std::vector<std::string>& args){
    // Check if the user provided a folder path
    if (args.empty()) {
        std::cerr << "Error: Missing folder path." << std::endl;
        return;
    }
    // Get the folder path from the first argument
    std::string folderPath = args[0];
    // Get the size of the folder
    ULARGE_INTEGER totalSize = {0};
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((folderPath + "\\*").c_str(), &findFileData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to open folder." << std::endl;
        return;
    }

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            totalSize.QuadPart += findFileData.nFileSizeHigh * (MAXDWORD + 1) + findFileData.nFileSizeLow;
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);
    
    FindClose(hFind);
    
    std::cout << "Total Size of Folder " << folderPath << ": " << totalSize.QuadPart / (1024) << " KB" << std::endl;
    std::cout << "Total Size of Folder " << folderPath << ": " << totalSize.QuadPart / (1024 * 1024) << " MB" << std::endl;
    std::cout << "Total Size of Folder " << folderPath << ": " << totalSize.QuadPart / (1024 * 1024 * 1024) << " GB" << std::endl;
    // std::cout << "Total Size of Folder " << folderPath << ": " << totalSize.QuadPart / (1024 * 1024 * 1024 * 1024) << " TB" << std::endl;
    // std::cout << "Total Size of Folder " << folderPath << ": " << totalSize.QuadPart / (1024 * 1024 * 1024 * 1024 * 1024) << " PB" << std::endl;
}

void showCapacityFile(const std::vector<std::string>& args){
    // Check if the user provided a file path
    if (args.empty()) {
        std::cerr << "Error: Missing file path." << std::endl;
        return;
    }
    // Get the file path from the first argument
    std::string filePath = args[0];
    // Get the size of the file
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(filePath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to open file." << std::endl;
        return;
    }
    ULARGE_INTEGER fileSize;
    fileSize.QuadPart = findFileData.nFileSizeHigh * (MAXDWORD + 1) + findFileData.nFileSizeLow;
    FindClose(hFind);
    std::cout << "Total Size of File " << filePath << ": " << fileSize.QuadPart / (1024) << " KB" << std::endl;
    std::cout << "Total Size of File " << filePath << ": " << fileSize.QuadPart / (1024 * 1024) << " MB" << std::endl;
    std::cout << "Total Size of File " << filePath << ": " << fileSize.QuadPart / (1024 * 1024 * 1024) << " GB" << std::endl;
    // std::cout << "Total Size of File " << filePath << ": " << fileSize.QuadPart / (1024 * 1024 * 1024 * 1024) << " TB" << std::endl;
}

#endif