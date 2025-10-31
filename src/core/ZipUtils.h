#pragma once

#include <string>
#include <vector>

namespace bik {

class ZipUtils {
public:
    // Create a zip archive from a directory
    static bool createZip(const std::string& sourceDir, const std::string& zipPath);
    
    // Extract a zip archive to a directory
    static bool extractZip(const std::string& zipPath, const std::string& destDir);
    
    // List files in a directory recursively
    static std::vector<std::string> listFiles(const std::string& dir);
    
    // Get file size
    static size_t getFileSize(const std::string& path);
    
    // Delete directory recursively
    static bool deleteDirectory(const std::string& path);
    
    // Copy directory recursively
    static bool copyDirectory(const std::string& source, const std::string& dest);
};

} // namespace bik
