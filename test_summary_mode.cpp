#include "src/Utility/ZstdCompressor.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::cout << "=== ZSTD Summary Mode Test ===" << std::endl;
    
    // Test 1: Normal mode (individual output)
    std::cout << "\n--- Test 1: Normal Mode (Individual Output) ---" << std::endl;
    std::string data1 = "Hello World! ";
    data1.append(100, 'A');
    std::vector<uint8_t> bytes1(data1.begin(), data1.end());
    
    auto compressed1 = ZstdCompressor::compress(bytes1);
    auto decompressed1 = ZstdCompressor::decompress(compressed1);
    
    // Test 2: Summary mode (collects stats, no individual output)
    std::cout << "\n--- Test 2: Summary Mode (Collects Stats) ---" << std::endl;
    ZstdCompressor::startSummaryMode();
    
    // Compress multiple items
    for (int i = 0; i < 5; i++) {
        std::string data = "Test data " + std::to_string(i) + " ";
        data.append(50, 'B');
        std::vector<uint8_t> bytes(data.begin(), data.end());
        
        auto compressed = ZstdCompressor::compress(bytes);
        auto decompressed = ZstdCompressor::decompress(compressed);
    }
    
    // Print summary
    ZstdCompressor::printSummary();
    
    // Stop summary mode
    ZstdCompressor::stopSummaryMode();
    
    // Test 3: Back to normal mode
    std::cout << "\n--- Test 3: Back to Normal Mode ---" << std::endl;
    std::string data3 = "Back to normal mode";
    std::vector<uint8_t> bytes3(data3.begin(), data3.end());
    
    auto compressed3 = ZstdCompressor::compress(bytes3);
    auto decompressed3 = ZstdCompressor::decompress(compressed3);
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}
