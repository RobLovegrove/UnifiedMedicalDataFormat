#include "ImageEncoder.hpp"
// #include "imageData.hpp"
#include "../../../Utility/Compression/CompressionType.hpp"

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <chrono>

ImageEncoder::ImageEncoder() {
    // Constructor - initialize with default factory
    factory = std::make_shared<CompressionFactory>();
    // Set default strategy to RAW (no compression)
    compressionStrategy = factory->createStrategy(CompressionType::RAW);
}

ImageEncoder::ImageEncoder(std::unique_ptr<CompressionStrategy> strategy) 
    : compressionStrategy(std::move(strategy)) {
    if (!compressionStrategy) {
        throw std::invalid_argument("Compression strategy cannot be null");
    }
    factory = std::make_shared<CompressionFactory>();
}

std::vector<uint8_t> ImageEncoder::compress(const std::vector<uint8_t>& rawData, 
                                            CompressionType encoding,
                                            int width, int height,
                                            uint8_t channels, uint8_t bitDepth) const {
    // Note: Individual frame timing removed - use total timing from ImageData instead
    
    // Create a temporary strategy for this specific compression type
    auto tempStrategy = factory->createStrategy(encoding);
    
    if (!tempStrategy) {
        std::cerr << "ERROR: Failed to create strategy for " << compressionTypeToString(encoding) << ", returning raw data!" << std::endl;
        std::cerr << "Available strategies: ";
        auto availableTypes = factory->getSupportedTypes();
        for (const auto& type : availableTypes) {
            std::cerr << compressionTypeToString(type) << " ";
        }
        std::cerr << std::endl;
        return rawData;
    }
    
    // Check if the strategy supports the given parameters
    if (!tempStrategy->supports(channels, bitDepth)) {
        std::cerr << "Warning: Compression type " << compressionTypeToString(encoding) 
                  << " may not support " << static_cast<int>(channels) << " channels and " 
                  << static_cast<int>(bitDepth) << " bit depth" << std::endl;
    }
    
    return tempStrategy->compress(rawData, width, height, channels, bitDepth);
}

std::vector<uint8_t> ImageEncoder::decompress(const std::vector<uint8_t>& compressedData,
                                              CompressionType encoding) const {
    // Note: Individual frame timing removed - use total timing from ImageData instead
    
    // Create a temporary strategy for this specific compression type
    auto tempStrategy = factory->createStrategy(encoding);
    
    if (!tempStrategy) {
        std::cerr << "Warning: Unsupported compression type " << compressionTypeToString(encoding) << ", returning as-is" << std::endl;
        return compressedData;
    }
    
    return tempStrategy->decompress(compressedData);
}





// Helper method to convert CompressionType enum to string
std::string ImageEncoder::compressionTypeToString(CompressionType type) const {
    switch (type) {
        case CompressionType::JPEG2000_LOSSLESS:
            return "JPEG2000_LOSSLESS";
        case CompressionType::PNG:
            return "PNG";
        case CompressionType::ZSTD:
            return "ZSTD";  // Note: ZSTD not implemented yet
        case CompressionType::RAW:
        default:
            return "RAW";
    }
}

// Strategy management methods
void ImageEncoder::setCompressionStrategy(std::unique_ptr<CompressionStrategy> strategy) {
    if (!strategy) {
        throw std::invalid_argument("Compression strategy cannot be null");
    }
    compressionStrategy = std::move(strategy);
}

void ImageEncoder::setCompressionStrategy(const std::string& type) {
    auto compressionType = stringToCompression(type);
    if (!compressionType.has_value()) {
        throw std::invalid_argument("Unsupported compression type: " + type);
    }
    auto newStrategy = factory->createStrategy(compressionType.value());
    if (!newStrategy) {
        throw std::invalid_argument("Failed to create strategy for compression type: " + type);
    }
    compressionStrategy = std::move(newStrategy);
}

std::string ImageEncoder::getCurrentCompressionType() const {
    if (!compressionStrategy) {
        return "NONE";
    }
    return compressionStrategy->getCompressionType();
}

bool ImageEncoder::supports(int channels, uint8_t bitDepth) const {
    if (!compressionStrategy) {
        return false;
    }
    return compressionStrategy->supports(channels, bitDepth);
}

bool ImageEncoder::testCompression() const {
    if (!compressionStrategy) {
        return false;
    }
    
    try {
        // Create a simple test image (2x2, 3 channels, 8-bit)
        std::vector<uint8_t> testData = {255, 0, 0,    // Red pixel
                                        0, 255, 0,     // Green pixel
                                        0, 0, 255,     // Blue pixel
                                        255, 255, 255}; // White pixel
        
        // Test compression
        auto compressed = compressionStrategy->compress(testData, 2, 2, 3, 8);
        if (compressed.empty()) {
            return false;
        }
        
        // Test decompression
        auto decompressed = compressionStrategy->decompress(compressed);
        if (decompressed.empty()) {
            return false;
        }
        
        // Verify data integrity (for lossless compression)
        if (compressionStrategy->getCompressionType() == "RAW" || 
            compressionStrategy->getCompressionType() == "PNG" ||
            compressionStrategy->getCompressionType() == "JPEG2000_LOSSLESS") {
            return testData == decompressed;
        }
        
        // For lossy compression, just check that we got some data back
        return !decompressed.empty();
        
    } catch (const std::exception& e) {
        std::cerr << "Compression test failed: " << e.what() << std::endl;
        return false;
    }
}

