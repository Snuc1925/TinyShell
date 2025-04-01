#include "command_manager.h"
#include <sstream>

void CommandManager::registerCommand(const std::string& name, std::function<void(const std::vector<std::string>&)> func) {
    commands[name] = func;
}

void CommandManager::executeCommand(const std::string& input) {
    std::istringstream iss(input);
    std::string commandName;
    std::vector<std::string> args;

    iss >> commandName;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    if (commands.find(commandName) != commands.end()) {
        commands[commandName](args);
    } else {
        std::cout << "Command not found: " << commandName << std::endl;
    }
}
