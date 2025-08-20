#include "src/Utility/ZstdCompressor.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::cout << "=== ZSTD Compression Statistics Test ===" << std::endl;
    
    // Test 1: Some repetitive data (should compress well)
    std::string repetitiveData = "Hello World! ";
    repetitiveData.append(1000, 'A');
    std::vector<uint8_t> data1(repetitiveData.begin(), repetitiveData.end());
    
    std::cout << "\n--- Test 1: Repetitive Data ---" << std::endl;
    std::cout << "Original size: " << data1.size() << " bytes" << std::endl;
    
    auto compressed1 = ZstdCompressor::compress(data1);
    auto decompressed1 = ZstdCompressor::decompress(compressed1);
    
    // Test 2: Random-like data (might not compress as well)
    std::vector<uint8_t> data2(1000);
    for (size_t i = 0; i < data2.size(); ++i) {
        data2[i] = static_cast<uint8_t>(i % 256);
    }
    
    std::cout << "\n--- Test 2: Random-like Data ---" << std::endl;
    std::cout << "Original size: " << data2.size() << " bytes" << std::endl;
    
    auto compressed2 = ZstdCompressor::compress(data2);
    auto decompressed2 = ZstdCompressor::decompress(compressed2);
    
    // Test 3: Different compression levels
    std::cout << "\n--- Test 3: Different Compression Levels ---" << std::endl;
    std::cout << "Original size: " << data1.size() << " bytes" << std::endl;
    
    auto compressed_low = ZstdCompressor::compressWithLevel(data1, 1);
    auto compressed_high = ZstdCompressor::compressWithLevel(data1, 22);
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}
