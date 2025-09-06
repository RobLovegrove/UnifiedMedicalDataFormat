#include <catch2/catch_all.hpp>
#include "DataModule/Image/Encoding/ImageEncoder.hpp"
#include <vector>
#include <cstdint>

TEST_CASE("ImageEncoder basic functionality", "[imageEncoder][basic]") {
    
    ImageEncoder encoder;
    
    SECTION("Can create encoder instance") {
        REQUIRE_NOTHROW(ImageEncoder{});
    }
    
    SECTION("Test OpenJPEG integration") {
        // Test if OpenJPEG is properly linked
        REQUIRE_NOTHROW(encoder.testOpenJPEGIntegration());
    }
}

TEST_CASE("ImageEncoder compression", "[imageEncoder][compression]") {
    
    ImageEncoder encoder;
    
    // Create test image data: 8x8 RGB image
    std::vector<uint8_t> testImage(8 * 8 * 3);
    for (size_t i = 0; i < testImage.size(); ++i) {
        testImage[i] = static_cast<uint8_t>(i % 256);
    }
    
    SECTION("JPEG2000 compression") {
        REQUIRE_NOTHROW([&]() {
            auto compressed = encoder.compressJPEG2000(testImage, 8, 8, 3, 8);
            REQUIRE(compressed.size() > 0);
            // Small images might not compress well, so just check it's valid
            REQUIRE(compressed.size() > 0);
        }());
    }
    
    SECTION("PNG compression") {
        REQUIRE_NOTHROW([&]() {
            auto compressed = encoder.compressPNG(testImage, 8, 8, 3, 8);
            REQUIRE(compressed.size() > 0);
            // PNG might not always compress small images
        }());
    }
    
    SECTION("Handles different bit depths") {
        // Test 16-bit data
        std::vector<uint8_t> testImage16bit(8 * 8 * 3 * 2); // 16-bit = 2 bytes per channel
        for (size_t i = 0; i < testImage16bit.size(); ++i) {
            testImage16bit[i] = static_cast<uint8_t>(i % 256);
        }
        
        REQUIRE_NOTHROW([&]() {
            auto compressed = encoder.compressJPEG2000(testImage16bit, 8, 8, 3, 16);
            REQUIRE(compressed.size() > 0);
        }());
    }
}

TEST_CASE("ImageEncoder decompression", "[imageEncoder][decompression]") {
    
    ImageEncoder encoder;
    
    // Create test image and compress it first
    std::vector<uint8_t> testImage(8 * 8 * 3);
    for (size_t i = 0; i < testImage.size(); ++i) {
        testImage[i] = static_cast<uint8_t>(i % 256);
    }
    
    SECTION("JPEG2000 round-trip") {
        auto compressed = encoder.compressJPEG2000(testImage, 8, 8, 3, 8);
        REQUIRE(compressed.size() > 0);
        
        auto decompressed = encoder.decompressJPEG2000(compressed);
        REQUIRE(decompressed.size() == testImage.size());
        
        // Data should match (JPEG2000 is lossless)
        REQUIRE(decompressed == testImage);
    }
    
    SECTION("PNG round-trip") {
        auto compressed = encoder.compressPNG(testImage, 8, 8, 3, 8);
        REQUIRE(compressed.size() > 0);
        
        auto decompressed = encoder.decompressPNG(compressed);
        REQUIRE(decompressed.size() == testImage.size());
        
        // Data should match (PNG is lossless)
        REQUIRE(decompressed == testImage);
    }
}

TEST_CASE("ImageEncoder error handling", "[imageEncoder][errors]") {
    
    ImageEncoder encoder;
    
    SECTION("Handles empty input gracefully") {
        std::vector<uint8_t> emptyData;
        
        REQUIRE_NOTHROW([&]() {
            auto result = encoder.compressJPEG2000(emptyData, 0, 0, 3, 8);
            // Should handle gracefully, might return empty result
        }());
    }
    
    SECTION("Handles invalid dimensions") {
        std::vector<uint8_t> testData(100);
        
        REQUIRE_NOTHROW([&]() {
            auto result = encoder.compressJPEG2000(testData, 0, 0, 3, 8);
            // Should handle gracefully
        }());
    }
}

TEST_CASE("ImageEncoder performance", "[imageEncoder][performance]") {
    
    ImageEncoder encoder;
    
    // Create larger test image: 64x64 RGB
    std::vector<uint8_t> testImage(64 * 64 * 3);
    for (size_t i = 0; i < testImage.size(); ++i) {
        testImage[i] = static_cast<uint8_t>(i % 256);
    }
    
    SECTION("Compression performance") {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto compressed = encoder.compressJPEG2000(testImage, 64, 64, 3, 8);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        REQUIRE(compressed.size() > 0);
        REQUIRE(duration.count() < 1000000); // Should complete in under 1 second
        
        // Calculate compression ratio
        double compressionRatio = static_cast<double>(compressed.size()) / testImage.size();
        REQUIRE(compressionRatio < 1.0); // Should actually compress
    }
}
