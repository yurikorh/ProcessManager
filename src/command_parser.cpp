#include "process_manager/command_parser.h"
#include <sstream>
#include <algorithm>

namespace ProcessManager {

CommandArgs CommandParser::parseCommand(const std::string& command_line) {
    // 检查是否包含shell语法
    if (needsShell(command_line)) {
        return {"/bin/bash", "-c", command_line};
    }
    
    // 普通命令解析
    CommandArgs tokens;
    std::string current_token;
    bool in_quotes = false;
    bool in_single_quotes = false;
    bool escaped = false;
    
    for (size_t i = 0; i < command_line.length(); ++i) {
        char c = command_line[i];
        
        if (escaped) {
            current_token += c;
            escaped = false;
            continue;
        }
        
        if (c == '\\') {
            escaped = true;
            current_token += c;
            continue;
        }
        
        if (c == '"' && !in_single_quotes) {
            in_quotes = !in_quotes;
            // 保留引号在token中
            current_token += c;
            continue;
        }
        
        if (c == '\'' && !in_quotes) {
            in_single_quotes = !in_single_quotes;
            // 保留引号在token中
            current_token += c;
            continue;
        }
        
        if ((c == ' ' || c == '\t') && !in_quotes && !in_single_quotes) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        } else {
            current_token += c;
        }
    }
    
    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }
    
    return tokens;
}

bool CommandParser::validateCommand(const CommandArgs& args) {
    return !args.empty() && !args[0].empty();
}

bool CommandParser::needsShell(const std::string& command) {
    // 检查是否包含shell特殊字符或关键字
    const std::vector<std::string> shell_operators = {
        "&&", "||", ";", "|", ">", "<", ">>", "<<",
        "source", "export", "cd", "$", "`", "$(", ")"
    };
    
    for (const auto& op : shell_operators) {
        if (command.find(op) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

} // namespace ProcessManager
