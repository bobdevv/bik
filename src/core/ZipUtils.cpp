#include "core/ZipUtils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <zlib.h>
#include <cstring>

namespace fs = std::filesystem;

namespace bik {

// Simple zip implementation using zlib
// For production, consider using libzip or minizip

bool ZipUtils::createZip(const std::string& sourceDir, const std::string& zipPath) {
    try {
        // For Windows, we'll use a simple approach with system commands
        // In production, use a proper zip library like libzip
        
        fs::path source = fs::absolute(sourceDir);
        fs::path dest = fs::absolute(zipPath);
        
        if (!fs::exists(source)) {
            std::cerr << "Source directory does not exist: " << source << std::endl;
            return false;
        }
        
        // Create parent directory for zip file
        fs::create_directories(dest.parent_path());
        
        // Use PowerShell Compress-Archive on Windows
        std::string cmd = "powershell -Command \"Compress-Archive -Path '" + 
                         source.string() + "\\*' -DestinationPath '" + 
                         dest.string() + "' -Force\"";
        
        int result = system(cmd.c_str());
        return result == 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating zip: " << e.what() << std::endl;
        return false;
    }
}

bool ZipUtils::extractZip(const std::string& zipPath, const std::string& destDir) {
    try {
        fs::path zip = fs::absolute(zipPath);
        fs::path dest = fs::absolute(destDir);
        
        if (!fs::exists(zip)) {
            std::cerr << "Zip file does not exist: " << zip << std::endl;
            return false;
        }
        
        // Create destination directory
        fs::create_directories(dest);
        
        // Use PowerShell Expand-Archive on Windows
        std::string cmd = "powershell -Command \"Expand-Archive -Path '" + 
                         zip.string() + "' -DestinationPath '" + 
                         dest.string() + "' -Force\"";
        
        int result = system(cmd.c_str());
        return result == 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error extracting zip: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> ZipUtils::listFiles(const std::string& dir) {
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error listing files: " << e.what() << std::endl;
    }
    
    return files;
}

size_t ZipUtils::getFileSize(const std::string& path) {
    try {
        return fs::file_size(path);
    } catch (const std::exception& e) {
        return 0;
    }
}

bool ZipUtils::deleteDirectory(const std::string& path) {
    try {
        return fs::remove_all(path) > 0;
    } catch (const std::exception& e) {
        std::cerr << "Error deleting directory: " << e.what() << std::endl;
        return false;
    }
}

bool ZipUtils::copyDirectory(const std::string& source, const std::string& dest) {
    try {
        fs::copy(source, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error copying directory: " << e.what() << std::endl;
        return false;
    }
}

} // namespace bik
