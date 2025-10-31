#pragma once

#include <string>
#include <vector>
#include <ctime>

namespace bik {

struct BackupInfo {
    std::string name;
    std::string path;
    std::time_t timestamp;
    size_t size;
};

class BackupManager {
public:
    BackupManager();
    ~BackupManager();

    // Initialize a project with backup directory
    bool initProject(const std::string& backupDir, const std::string& projectDir);
    
    // Create a backup
    bool createBackup(const std::string& name = "");
    
    // List all backups
    std::vector<BackupInfo> listBackups() const;
    
    // Load a specific backup
    bool loadBackup(const std::string& name);
    
    // Load the most recent backup
    bool loadLastBackup();
    
    // Clean all backups
    bool cleanAllBackups();
    
    // Wipe old backups (keep only the most recent)
    bool wipeOldBackups();
    
    // Get current project directory
    std::string getProjectDir() const;
    
    // Get backup directory
    std::string getBackupDir() const;
    
    // Check if project is initialized
    bool isInitialized() const;

private:
    std::string generateBackupName(const std::string& baseName) const;
    std::string getConfigPath() const;
    bool loadConfig();
    bool saveConfig();
    
    std::string m_projectDir;
    std::string m_backupDir;
    std::string m_projectName;
    bool m_initialized;
};

} // namespace bik
