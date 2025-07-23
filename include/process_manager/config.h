#pragma once
#include "ylt/struct_yaml/yaml_reader.h"
#include <string>
#include <vector>
#include "ylt/easylog.hpp"

namespace ProcessManager {

struct ModuleConfig {
    std::string command;
    std::optional<std::vector<std::string>> depends_on;
    bool restart_on_failure;
    std::optional<std::map<std::string, std::string>> env;
};
YLT_REFL(ModuleConfig, command, depends_on, restart_on_failure, env);

struct ModulesConfig {
    std::map<std::string, ModuleConfig> modules;
};
YLT_REFL(ModulesConfig, modules);

// 函数声明
ModulesConfig load_config(const std::string& config_file);

} // namespace ProcessManager

