#pragma once

#include <string>
#include <map>

namespace bik {

class ProjectConfig {
public:
    ProjectConfig();
    ~ProjectConfig();

    bool load(const std::string& path);
    bool save(const std::string& path);
    
    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key, const std::string& defaultValue = "") const;
    
    bool has(const std::string& key) const;

private:
    std::map<std::string, std::string> m_data;
};

} // namespace bik
