#include "core/ProjectConfig.h"
#include <fstream>
#include <sstream>

namespace bik {

ProjectConfig::ProjectConfig() {
}

ProjectConfig::~ProjectConfig() {
}

bool ProjectConfig::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    m_data.clear();
    std::string line;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse key=value
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            m_data[key] = value;
        }
    }
    
    return true;
}

bool ProjectConfig::save(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# Bik Project Configuration\n";
    for (const auto& pair : m_data) {
        file << pair.first << "=" << pair.second << "\n";
    }
    
    return true;
}

void ProjectConfig::set(const std::string& key, const std::string& value) {
    m_data[key] = value;
}

std::string ProjectConfig::get(const std::string& key, const std::string& defaultValue) const {
    auto it = m_data.find(key);
    if (it != m_data.end()) {
        return it->second;
    }
    return defaultValue;
}

bool ProjectConfig::has(const std::string& key) const {
    return m_data.find(key) != m_data.end();
}

} // namespace bik
