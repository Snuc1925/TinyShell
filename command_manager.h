#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

class CommandManager {
private:
    std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> commands;

public:
    void registerCommand(const std::string& name, std::function<void(const std::vector<std::string>&)> func);
    void executeCommand(const std::string& input);
};

#endif
