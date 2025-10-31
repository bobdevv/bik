#pragma once

#include <string>
#include <vector>

namespace bik {

class BackupManager;

class CommandHandler {
public:
    CommandHandler();
    ~CommandHandler();

    int execute(int argc, char* argv[]);

private:
    void printUsage() const;
    void printVersion() const;
    
    int handleProjectCommand(const std::vector<std::string>& args);
    int handleBackupCommand(const std::vector<std::string>& args);
    int handleCleanCommand(const std::vector<std::string>& args);
    int handleWipeOldCommand(const std::vector<std::string>& args);
    int handleLoadCommand(const std::vector<std::string>& args);
    int handleGuiCommand(const std::vector<std::string>& args);
    
    std::string findArgValue(const std::vector<std::string>& args, 
                            const std::string& flag) const;
    bool hasFlag(const std::vector<std::string>& args, 
                const std::string& flag) const;
};

} // namespace bik
