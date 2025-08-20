#include "ZstdCompressor.hpp"
#include <zstd.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

// Initialize static variables
bool ZstdCompressor::summaryMode = false;
size_t ZstdCompressor::totalCompressions = 0;
size_t ZstdCompressor::totalDecompressions = 0;
size_t ZstdCompressor::totalOriginalSize = 0;
size_t ZstdCompressor::totalCompressedSize = 0;
int ZstdCompressor::compressionLevel = 0;

std::vector<uint8_t> ZstdCompressor::compress(const std::vector<uint8_t>& data) {
    // Use higher compression level for better ratios
    return compressWithLevel(data, 15);  // Changed from ZSTD_CLEVEL_DEFAULT (3) to 15
}

std::vector<uint8_t> ZstdCompressor::compressWithLevel(const std::vector<uint8_t>& data, int level) {
    if (data.empty()) {
        return {};
    }
    
    // Validate compression level
    if (level < ZSTD_minCLevel() || level > ZSTD_maxCLevel()) {
        throw std::runtime_error("Invalid compression level: " + std::to_string(level) + 
                               " (valid range: " + std::to_string(ZSTD_minCLevel()) + 
                               " to " + std::to_string(ZSTD_maxCLevel()) + ")");
    }
    
    // Calculate maximum compressed size
    size_t maxCompressedSize = ZSTD_compressBound(data.size());
    std::vector<uint8_t> compressed(maxCompressedSize);
    
    // Compress the data
    size_t actualCompressedSize = ZSTD_compress(
        compressed.data(), maxCompressedSize,
        data.data(), data.size(),
        level
    );
    
    // Check for compression errors
    if (ZSTD_isError(actualCompressedSize)) {
        throw std::runtime_error("ZSTD compression failed: " + 
                               std::string(ZSTD_getErrorName(actualCompressedSize)));
    }
    
    // Resize to actual compressed size
    compressed.resize(actualCompressedSize);
    
    // Handle summary mode vs individual output
    if (summaryMode) {
        totalCompressions++;
        totalOriginalSize += data.size();
        totalCompressedSize += actualCompressedSize;
        // Track the compression level used (use the highest level if multiple compressions)
        if (level > compressionLevel) {
            compressionLevel = level;
        }
    } else {
        // Output compression statistics
        double ratio = getCompressionRatio(data.size(), actualCompressedSize);
        std::cout << "ZSTD Compression: " << std::fixed << std::setprecision(1)
                  << data.size() << " bytes -> " << actualCompressedSize << " bytes "
                  << "(" << ratio << "% of original, level " << level << ")" << std::endl;
    }
    
    return compressed;
}

std::vector<uint8_t> ZstdCompressor::decompress(const std::vector<uint8_t>& compressedData) {
    if (compressedData.empty()) {
        return {};
    }
    
    // Get the original size from the ZSTD frame header
    unsigned long long originalSize = ZSTD_getFrameContentSize(
        compressedData.data(), compressedData.size()
    );
    
    // Check if we can determine the original size
    if (originalSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        throw std::runtime_error("Cannot determine original size from ZSTD frame (streaming mode)");
    }
    if (originalSize == ZSTD_CONTENTSIZE_ERROR) {
        throw std::runtime_error("Invalid ZSTD frame or corrupted data");
    }
    if (originalSize == 0) {
        return {}; // Empty original data
    }
    
    // Allocate buffer for decompressed data
    std::vector<uint8_t> decompressed(originalSize);
    
    // Decompress the data
    size_t actualDecompressedSize = ZSTD_decompress(
        decompressed.data(), originalSize,
        compressedData.data(), compressedData.size()
    );
    
    // Check for decompression errors
    if (ZSTD_isError(actualDecompressedSize)) {
        throw std::runtime_error("ZSTD decompression failed: " + 
                               std::string(ZSTD_getErrorName(actualDecompressedSize)));
    }
    
    // Verify the decompressed size matches expected
    if (actualDecompressedSize != originalSize) {
        throw std::runtime_error("Decompressed size mismatch: expected " + 
                               std::to_string(originalSize) + ", got " + 
                               std::to_string(actualDecompressedSize));
    }
    
    // Handle summary mode vs individual output
    if (summaryMode) {
        totalDecompressions++;
        totalOriginalSize += actualDecompressedSize;
        totalCompressedSize += compressedData.size();
    } else {
        // Output decompression statistics
        std::cout << "ZSTD Decompression: " << compressedData.size() << " bytes -> " 
                  << actualDecompressedSize << " bytes" << std::endl;
    }
    
    return decompressed;
}

double ZstdCompressor::getCompressionRatio(size_t originalSize, size_t compressedSize) {
    if (originalSize == 0) {
        return 0.0;
    }
    
    // Calculate percentage of original size
    double ratio = (static_cast<double>(compressedSize) / static_cast<double>(originalSize)) * 100.0;
    return ratio;
}

double ZstdCompressor::getCompressionRatio(const std::vector<uint8_t>& original, 
                                          const std::vector<uint8_t>& compressed) {
    return getCompressionRatio(original.size(), compressed.size());
}

bool ZstdCompressor::shouldCompress(const std::vector<uint8_t>& data, size_t minSize) {
    // Don't compress if data is too small
    if (data.size() < minSize) {
        return false;
    }
    
    // Don't compress if data is already very small
    if (data.size() < 64) {
        return false;
    }
    
    // Check if data has some entropy (not all zeros or repeated patterns)
    if (data.size() > 0) {
        uint8_t firstByte = data[0];
        bool allSame = std::all_of(data.begin(), data.end(), 
                                  [firstByte](uint8_t b) { return b == firstByte; });
        if (allSame && data.size() < 256) {
            return false; // Very repetitive small data
        }
    }
    
    return true;
}

std::string ZstdCompressor::getVersion() {
    std::ostringstream oss;
    oss << "ZSTD " << ZSTD_VERSION_MAJOR << "." 
        << ZSTD_VERSION_MINOR << "." 
        << ZSTD_VERSION_RELEASE;
    return oss.str();
}

// Summary mode methods
void ZstdCompressor::startSummaryMode() {
    summaryMode = true;
    totalCompressions = 0;
    totalDecompressions = 0;
    totalOriginalSize = 0;
    totalCompressedSize = 0;
    compressionLevel = 0;
}

void ZstdCompressor::stopSummaryMode() {
    summaryMode = false;
}

bool ZstdCompressor::isSummaryMode() {
    return summaryMode;
}

void ZstdCompressor::printSummary() {
    if (totalCompressions > 0 || totalDecompressions > 0) {
        std::cout << "\n=== ZSTD COMPRESSION SUMMARY ===" << std::endl;
        
        if (totalCompressions > 0) {
            double avgRatio = getCompressionRatio(totalOriginalSize, totalCompressedSize);
            std::cout << "Compressions: " << totalCompressions << " operations" << std::endl;
            std::cout << "Total original: " << totalOriginalSize << " bytes" << std::endl;
            std::cout << "Total compressed: " << totalCompressedSize << " bytes" << std::endl;
            std::cout << "Average compression: " << std::fixed << std::setprecision(1) 
                      << avgRatio << "% of original" << std::endl;
            std::cout << "Compression level: " << compressionLevel << std::endl;
        }
        
        if (totalDecompressions > 0) {
            std::cout << "Decompressions: " << totalDecompressions << " operations" << std::endl;
        }
        
        std::cout << "================================" << std::endl;
    }
}
