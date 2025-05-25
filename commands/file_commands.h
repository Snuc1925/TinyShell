// commands/file_commands.h
#ifndef FILE_COMMANDS_H
#define FILE_COMMANDS_H

#include "../command_manager.h"
#include <windows.h>
#include <iostream>
#include <string>

void listFiles(const std::vector<std::string> &args)
{
    std::string path = (args.empty()) ? "." : args[0];
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error: Unable to open directory." << std::endl;
        return;
    }

    do
    {
        std::string fileName = findFileData.cFileName;
        if (fileName != "." && fileName != "..")
        { // Bỏ qua "." và ".."
            std::cout << fileName << std::endl;
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void makeDirectory(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        std::cerr << "Error: Missing directory name." << std::endl;
        return;
    }

    std::string dirName = args[0];
    if (CreateDirectoryA(dirName.c_str(), NULL))
    {
        std::cout << "Directory created: " << dirName << std::endl;
    }
    else
    {
        std::cerr << "Error: Unable to create directory." << std::endl;
    }
}

void changeDirectory(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        std::cerr << "Error: Missing directory argument." << std::endl;
        return;
    }

    if (!SetCurrentDirectoryA(args[0].c_str()))
    {
        std::cerr << "Error: Unable to change directory to " << args[0] << std::endl;
    }
}

void touchCommand(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        std::cerr << "Error: Missing file name." << std::endl;
        return;
    }
    HANDLE hFile = CreateFileA(args[0].c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_EXISTS)
            std::cerr << "Error: File already exists." << std::endl;
        else
            std::cerr << "Error: Unable to create file." << std::endl;
        return;
    }
    CloseHandle(hFile);
}

void catCommand(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        std::cerr << "Error: Missing file name." << std::endl;
        return;
    }
    FILE *file = fopen(args[0].c_str(), "r");
    if (!file)
    {
        std::cerr << "Error: Unable to open file." << std::endl;
        return;
    }
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file))
    {
        std::cout << buffer;
    }
    std::cout << std::endl;
    fclose(file);
}

void headCommand(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        std::cerr << "Error: Missing file name." << std::endl;
        return;
    }
    FILE *file = fopen(args[0].c_str(), "r");
    if (!file)
    {
        std::cerr << "Error: Unable to open file." << std::endl;
        return;
    }
    char buffer[256];
    int line = 0;
    while (fgets(buffer, sizeof(buffer), file) && line < 10)
    {
        std::cout << buffer;
        ++line;
    }
    fclose(file);
}

#endif
