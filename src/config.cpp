#include "process_manager/config.h"
#include <fstream>
#include <sstream>

namespace ProcessManager {
ModulesConfig load_config(const std::string& config_file) {
    std::string yaml;
    // 读取配置文件内容
    std::ifstream file(config_file);
    if (!file.is_open()) {
        ELOG_ERROR << "Failed to open config file: " << config_file;
        return {};
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    yaml = buffer.str();
    file.close();
    
    ELOG_INFO << "YAML content: " << yaml;
    
    // 解析YAML内容
    ModulesConfig config;
    try {
        struct_yaml::from_yaml(config, yaml);
    } catch (const std::exception& e) {
        ELOG_ERROR << "Failed to parse YAML: " << e.what();
        return {};
    }
    
    if (config.modules.empty()) {
        ELOG_ERROR << "No modules found in configuration.";
        return {};
    }
    return config;
}
}