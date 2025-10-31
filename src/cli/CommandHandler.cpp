#include "cli/CommandHandler.h"
#include "core/BackupManager.h"
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

namespace bik {

CommandHandler::CommandHandler() {
}

CommandHandler::~CommandHandler() {
}

int CommandHandler::execute(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }
    
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }
    
    std::string command = args[0];
    
    if (command == "project") {
        return handleProjectCommand(args);
    } else if (command == "backup") {
        return handleBackupCommand(args);
    } else if (command == "clean") {
        return handleCleanCommand(args);
    } else if (command == "wipeold") {
        return handleWipeOldCommand(args);
    } else if (command == "load") {
        return handleLoadCommand(args);
    } else if (command == "--version" || command == "-v") {
        printVersion();
        return 0;
    } else if (command == "--help" || command == "-h") {
        printUsage();
        return 0;
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        printUsage();
        return 1;
    }
}

void CommandHandler::printUsage() const {
    std::cout << "Bik - Simple Backup Manager v1.0.0\n\n";
    std::cout << "Usage: bik <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  project -b <backup_dir> [-n <name>]  Initialize project with backup directory\n";
    std::cout << "  backup [-n <name>]                    Create a new backup\n";
    std::cout << "  clean                                 Delete all backups\n";
    std::cout << "  wipeold                               Delete all backups except the most recent\n";
    std::cout << "  load [-last]                          Load a backup (interactive or last)\n";
    std::cout << "  --help, -h                            Show this help message\n";
    std::cout << "  --version, -v                         Show version information\n";
    std::cout << "\nExamples:\n";
    std::cout << "  bik project -b /path/to/backups\n";
    std::cout << "  bik project -b C:\\Backups -n my-project\n";
    std::cout << "  bik backup\n";
    std::cout << "  bik backup -n working-version-1\n";
    std::cout << "  bik load\n";
    std::cout << "  bik load -last\n";
}

void CommandHandler::printVersion() const {
    std::cout << "Bik v1.0.0\n";
    std::cout << "Simple and reliable backup manager for code projects\n";
}

int CommandHandler::handleProjectCommand(const std::vector<std::string>& args) {
    std::string backupDir = findArgValue(args, "-b");
    std::string name = findArgValue(args, "-n");
    
    if (backupDir.empty()) {
        std::cerr << "Error: -b <backup_dir> is required\n";
        std::cerr << "Usage: bik project -b <backup_dir> [-n <name>]\n";
        return 1;
    }
    
    BackupManager manager;
    std::string projectDir = fs::current_path().string();
    
    if (!manager.initProject(backupDir, projectDir)) {
        return 1;
    }
    
    std::cout << "Project initialized successfully!\n";
    std::cout << "Project directory: " << manager.getProjectDir() << "\n";
    std::cout << "Backup directory: " << manager.getBackupDir() << "\n";
    
    // Create initial backup if name is provided
    if (!name.empty()) {
        std::cout << "\nCreating initial backup...\n";
        if (manager.createBackup(name)) {
            std::cout << "Initial backup created: " << name << "\n";
        }
    }
    
    return 0;
}

int CommandHandler::handleBackupCommand(const std::vector<std::string>& args) {
    std::string name = findArgValue(args, "-n");
    
    BackupManager manager;
    if (!manager.isInitialized()) {
        std::cerr << "Error: Project not initialized. Run 'bik project -b <backup_dir>' first.\n";
        return 1;
    }
    
    if (manager.createBackup(name)) {
        return 0;
    }
    return 1;
}

int CommandHandler::handleCleanCommand(const std::vector<std::string>& args) {
    BackupManager manager;
    if (!manager.isInitialized()) {
        std::cerr << "Error: Project not initialized.\n";
        return 1;
    }
    
    if (manager.cleanAllBackups()) {
        return 0;
    }
    return 1;
}

int CommandHandler::handleWipeOldCommand(const std::vector<std::string>& args) {
    BackupManager manager;
    if (!manager.isInitialized()) {
        std::cerr << "Error: Project not initialized.\n";
        return 1;
    }
    
    if (manager.wipeOldBackups()) {
        return 0;
    }
    return 1;
}

int CommandHandler::handleLoadCommand(const std::vector<std::string>& args) {
    BackupManager manager;
    if (!manager.isInitialized()) {
        std::cerr << "Error: Project not initialized.\n";
        return 1;
    }
    
    bool loadLast = hasFlag(args, "-last");
    
    if (loadLast) {
        if (manager.loadLastBackup()) {
            return 0;
        }
        return 1;
    }
    
    // Interactive mode
    auto backups = manager.listBackups();
    if (backups.empty()) {
        std::cout << "No backups found.\n";
        return 1;
    }
    
    std::cout << "\nAvailable backups:\n";
    std::cout << std::string(80, '-') << "\n";
    
    for (size_t i = 0; i < backups.size(); i++) {
        std::time_t t = backups[i].timestamp;
        std::tm* tm = std::localtime(&t);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
        
        double sizeMB = backups[i].size / (1024.0 * 1024.0);
        
        std::cout << std::setw(3) << (i + 1) << ". " 
                  << std::setw(30) << std::left << backups[i].name
                  << " | " << timeStr
                  << " | " << std::fixed << std::setprecision(2) << sizeMB << " MB\n";
    }
    
    std::cout << std::string(80, '-') << "\n";
    std::cout << "Enter backup number to load (0 to cancel): ";
    
    int choice;
    std::cin >> choice;
    std::cin.ignore(); // Clear newline
    
    if (choice < 1 || choice > static_cast<int>(backups.size())) {
        std::cout << "Cancelled.\n";
        return 0;
    }
    
    if (manager.loadBackup(backups[choice - 1].name)) {
        return 0;
    }
    return 1;
}


std::string CommandHandler::findArgValue(const std::vector<std::string>& args, 
                                         const std::string& flag) const {
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == flag && i + 1 < args.size()) {
            return args[i + 1];
        }
    }
    return "";
}

bool CommandHandler::hasFlag(const std::vector<std::string>& args, 
                             const std::string& flag) const {
    for (const auto& arg : args) {
        if (arg == flag) {
            return true;
        }
    }
    return false;
}

} // namespace bik
