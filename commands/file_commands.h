// commands/file_commands.h
#ifndef FILE_COMMANDS_H
#define FILE_COMMANDS_H

#include "../command_manager.h"
#include <windows.h>
#include <iostream>
#include <string>

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

void clearCLS(const std::vector<std::string>& args) {
    // Like the 'cls' command in Windows, this function clears the console screen.
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD written;
    if (hConsole == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to get console handle." << std::endl;
        return;
    }
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        std::cerr << "Error: Unable to get console buffer info." << std::endl;
        return;
    }
    DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
    if (!FillConsoleOutputCharacterA(hConsole, ' ', size, coord, &written)) {
        std::cerr << "Error: Unable to fill console output." << std::endl;
        return;
    }
    if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, size, coord, &written)) {
        std::cerr << "Error: Unable to fill console attributes." << std::endl;
        return;
    }
    SetConsoleCursorPosition(hConsole, coord);
    std::cout << "Console cleared." << std::endl;
}

#endif
