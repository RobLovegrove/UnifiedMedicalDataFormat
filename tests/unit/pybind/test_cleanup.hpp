#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <iostream>

namespace test_cleanup {

class TestFileManager {
private:
    std::vector<std::string> created_files;
    static TestFileManager* instance;
    
    TestFileManager() = default;
    
public:
    // Singleton pattern
    static TestFileManager& getInstance() {
        static TestFileManager instance;
        return instance;
    }
    
    // Add a file to the cleanup list
    void add_file(const std::string& filename) {
        created_files.push_back(filename);
    }
    
    // Clean up all tracked files
    void cleanup_all() {
        for (const auto& filename : created_files) {
            try {
                if (std::filesystem::exists(filename)) {
                    std::filesystem::remove(filename);
                    std::cout << "DEBUG: Cleaned up test file: " << filename << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "DEBUG: Warning - could not remove test file " << filename 
                         << ": " << e.what() << std::endl;
            }
        }
        created_files.clear();
    }
    
    // Clean up files matching a pattern
    void cleanup_pattern(const std::string& pattern) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(".")) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (filename.find(pattern) != std::string::npos && 
                        filename.ends_with(".umdf")) {
                        std::filesystem::remove(entry.path());
                        std::cout << "DEBUG: Cleaned up pattern file: " << filename << std::endl;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cout << "DEBUG: Warning - could not clean up pattern " << pattern 
                     << ": " << e.what() << std::endl;
        }
    }
    
    // Get the list of created files
    const std::vector<std::string>& get_created_files() const {
        return created_files;
    }
};

// Global cleanup function for all test files
void cleanup_all_test_files();

} // namespace test_cleanup
