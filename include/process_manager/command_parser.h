#pragma once
#include "types.h"
#include <string>

namespace ProcessManager {

class CommandParser {
public:
    static CommandArgs parseCommand(const std::string& command_line);
    static bool validateCommand(const CommandArgs& args);
    
private:
    static bool needsShell(const std::string& command);
};

} // namespace ProcessManager
