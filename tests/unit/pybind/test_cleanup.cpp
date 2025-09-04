#include "test_cleanup.hpp"
#include <cstdlib>

namespace test_cleanup {

// Global cleanup function that runs at program exit
void cleanup_all_test_files() {
    std::cout << "DEBUG: Starting global test file cleanup..." << std::endl;
    
    try {
        // Clean up all .umdf files that start with "test_"
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.starts_with("test_") && filename.ends_with(".umdf")) {
                    std::filesystem::remove(entry.path());
                    std::cout << "DEBUG: Cleaned up test file: " << filename << std::endl;
                }
            }
        }
        
        // Also clean up any .tmp files
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.ends_with(".umdf.tmp")) {
                    std::filesystem::remove(entry.path());
                    std::cout << "DEBUG: Cleaned up temp file: " << filename << std::endl;
                }
            }
        }
        
        std::cout << "DEBUG: Global test file cleanup completed." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "DEBUG: Warning - error during global cleanup: " << e.what() << std::endl;
    }
}

} // namespace test_cleanup

// Register cleanup function to run at program exit
static void register_cleanup() {
    std::atexit([]() {
        test_cleanup::cleanup_all_test_files();
    });
}

// This will run when the file is loaded
static int dummy = (register_cleanup(), 0);
