#include "core/BackupManager.h"
#include "core/ProjectConfig.h"
#include "core/ZipUtils.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace bik {

BackupManager::BackupManager() : m_initialized(false) {
    loadConfig();
}

BackupManager::~BackupManager() {
}

bool BackupManager::initProject(const std::string& backupDir, const std::string& projectDir) {
    try {
        // Resolve paths
        fs::path projPath = fs::absolute(projectDir);
        fs::path backupPath = fs::absolute(backupDir);
        
        if (!fs::exists(projPath)) {
            std::cerr << "Error: Project directory does not exist: " << projPath << std::endl;
            return false;
        }
        
        // Create backup directory if it doesn't exist
        if (!fs::exists(backupPath)) {
            fs::create_directories(backupPath);
        }
        
        m_projectDir = projPath.string();
        m_backupDir = backupPath.string();
        m_projectName = projPath.filename().string();
        m_initialized = true;
        
        return saveConfig();
    } catch (const std::exception& e) {
        std::cerr << "Error initializing project: " << e.what() << std::endl;
        return false;
    }
}

bool BackupManager::createBackup(const std::string& name) {
    if (!m_initialized) {
        std::cerr << "Error: Project not initialized. Use 'bik project -b <backup_dir>' first." << std::endl;
        return false;
    }
    
    try {
        std::string backupName = name.empty() ? generateBackupName(m_projectName) : name;
        fs::path zipPath = fs::path(m_backupDir) / (backupName + ".zip");
        
        std::cout << "Creating backup: " << backupName << std::endl;
        std::cout << "Source: " << m_projectDir << std::endl;
        std::cout << "Destination: " << zipPath << std::endl;
        
        if (!ZipUtils::createZip(m_projectDir, zipPath.string())) {
            std::cerr << "Error: Failed to create backup" << std::endl;
            return false;
        }
        
        std::cout << "Backup created successfully!" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating backup: " << e.what() << std::endl;
        return false;
    }
}

std::vector<BackupInfo> BackupManager::listBackups() const {
    std::vector<BackupInfo> backups;
    
    if (!m_initialized || !fs::exists(m_backupDir)) {
        return backups;
    }
    
    try {
        for (const auto& entry : fs::directory_iterator(m_backupDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".zip") {
                BackupInfo info;
                info.name = entry.path().stem().string();
                info.path = entry.path().string();
                
                auto ftime = fs::last_write_time(entry.path());
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                info.timestamp = std::chrono::system_clock::to_time_t(sctp);
                
                info.size = fs::file_size(entry.path());
                backups.push_back(info);
            }
        }
        
        // Sort by timestamp (newest first)
        std::sort(backups.begin(), backups.end(), 
                 [](const BackupInfo& a, const BackupInfo& b) {
                     return a.timestamp > b.timestamp;
                 });
    } catch (const std::exception& e) {
        std::cerr << "Error listing backups: " << e.what() << std::endl;
    }
    
    return backups;
}

bool BackupManager::loadBackup(const std::string& name) {
    if (!m_initialized) {
        std::cerr << "Error: Project not initialized." << std::endl;
        return false;
    }
    
    try {
        fs::path zipPath = fs::path(m_backupDir) / (name + ".zip");
        
        if (!fs::exists(zipPath)) {
            std::cerr << "Error: Backup not found: " << name << std::endl;
            return false;
        }
        
        std::cout << "Loading backup: " << name << std::endl;
        std::cout << "This will replace current directory contents." << std::endl;
        std::cout << "Continue? (y/n): ";
        
        std::string response;
        std::getline(std::cin, response);
        
        if (response != "y" && response != "Y") {
            std::cout << "Cancelled." << std::endl;
            return false;
        }
        
        // Create temporary directory
        fs::path tempDir = fs::temp_directory_path() / ("bik_restore_" + std::to_string(std::time(nullptr)));
        fs::create_directories(tempDir);
        
        // Extract to temp
        if (!ZipUtils::extractZip(zipPath.string(), tempDir.string())) {
            std::cerr << "Error: Failed to extract backup" << std::endl;
            fs::remove_all(tempDir);
            return false;
        }
        
        // Remove current directory contents (except .bik config)
        for (const auto& entry : fs::directory_iterator(m_projectDir)) {
            if (entry.path().filename() != ".bik") {
                fs::remove_all(entry.path());
            }
        }
        
        // Copy from temp to project dir
        for (const auto& entry : fs::directory_iterator(tempDir)) {
            fs::path dest = fs::path(m_projectDir) / entry.path().filename();
            if (entry.is_directory()) {
                fs::copy(entry.path(), dest, fs::copy_options::recursive);
            } else {
                fs::copy(entry.path(), dest);
            }
        }
        
        // Clean up temp
        fs::remove_all(tempDir);
        
        std::cout << "Backup loaded successfully!" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading backup: " << e.what() << std::endl;
        return false;
    }
}

bool BackupManager::loadLastBackup() {
    auto backups = listBackups();
    if (backups.empty()) {
        std::cerr << "Error: No backups found" << std::endl;
        return false;
    }
    
    return loadBackup(backups[0].name);
}

bool BackupManager::cleanAllBackups() {
    if (!m_initialized) {
        std::cerr << "Error: Project not initialized." << std::endl;
        return false;
    }
    
    try {
        std::cout << "This will delete all backups. Continue? (y/n): ";
        std::string response;
        std::getline(std::cin, response);
        
        if (response != "y" && response != "Y") {
            std::cout << "Cancelled." << std::endl;
            return false;
        }
        
        int count = 0;
        for (const auto& entry : fs::directory_iterator(m_backupDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".zip") {
                fs::remove(entry.path());
                count++;
            }
        }
        
        std::cout << "Deleted " << count << " backup(s)." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error cleaning backups: " << e.what() << std::endl;
        return false;
    }
}

bool BackupManager::wipeOldBackups() {
    if (!m_initialized) {
        std::cerr << "Error: Project not initialized." << std::endl;
        return false;
    }
    
    try {
        auto backups = listBackups();
        if (backups.size() <= 1) {
            std::cout << "No old backups to delete." << std::endl;
            return true;
        }
        
        std::cout << "This will delete " << (backups.size() - 1) << " old backup(s). Continue? (y/n): ";
        std::string response;
        std::getline(std::cin, response);
        
        if (response != "y" && response != "Y") {
            std::cout << "Cancelled." << std::endl;
            return false;
        }
        
        // Keep the first one (newest), delete the rest
        for (size_t i = 1; i < backups.size(); i++) {
            fs::remove(backups[i].path);
        }
        
        std::cout << "Deleted " << (backups.size() - 1) << " old backup(s)." << std::endl;
        std::cout << "Kept: " << backups[0].name << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error wiping old backups: " << e.what() << std::endl;
        return false;
    }
}

std::string BackupManager::getProjectDir() const {
    return m_projectDir;
}

std::string BackupManager::getBackupDir() const {
    return m_backupDir;
}

bool BackupManager::isInitialized() const {
    return m_initialized;
}

std::string BackupManager::generateBackupName(const std::string& baseName) const {
    // Find the next available backup number
    int maxNum = -1;
    
    if (fs::exists(m_backupDir)) {
        for (const auto& entry : fs::directory_iterator(m_backupDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".zip") {
                std::string name = entry.path().stem().string();
                
                // Check if it matches pattern: baseName-backup-N
                std::string prefix = baseName + "-backup-";
                if (name.find(prefix) == 0) {
                    std::string numStr = name.substr(prefix.length());
                    try {
                        int num = std::stoi(numStr);
                        maxNum = std::max(maxNum, num);
                    } catch (...) {
                        // Not a number, skip
                    }
                }
            }
        }
    }
    
    return baseName + "-backup-" + std::to_string(maxNum + 1);
}

std::string BackupManager::getConfigPath() const {
    return fs::path(fs::current_path()) / ".bik" / "config.txt";
}

bool BackupManager::loadConfig() {
    try {
        std::string configPath = getConfigPath();
        if (!fs::exists(configPath)) {
            return false;
        }
        
        ProjectConfig config;
        if (!config.load(configPath)) {
            return false;
        }
        
        m_projectDir = config.get("project_dir");
        m_backupDir = config.get("backup_dir");
        m_projectName = config.get("project_name");
        
        m_initialized = !m_projectDir.empty() && !m_backupDir.empty();
        return m_initialized;
    } catch (const std::exception& e) {
        return false;
    }
}

bool BackupManager::saveConfig() {
    try {
        fs::path configDir = fs::path(m_projectDir) / ".bik";
        if (!fs::exists(configDir)) {
            fs::create_directories(configDir);
        }
        
        ProjectConfig config;
        config.set("project_dir", m_projectDir);
        config.set("backup_dir", m_backupDir);
        config.set("project_name", m_projectName);
        
        std::string configPath = (configDir / "config.txt").string();
        return config.save(configPath);
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}

} // namespace bik
