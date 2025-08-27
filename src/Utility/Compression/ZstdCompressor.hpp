#ifndef ZSTDCOMPRESSOR_HPP
#define ZSTDCOMPRESSOR_HPP

#include <vector>
#include <cstdint>
#include <string>

/**
 * @brief ZSTD compression utility class for header data (metadata + string buffer)
 * 
 * This class provides static methods for compressing and decompressing data using ZSTD.
 * It's designed to be used for compressing combined metadata and string buffer data
 * in UMDF modules, where cross-pattern recognition can improve compression ratios.
 * 
 * The class automatically tracks all compression/decompression operations and provides
 * summary statistics when requested.
 */
class ZstdCompressor {
public:
    /**
     * @brief Compress data using ZSTD with default compression level
     * 
     * @param data Raw data to compress
     * @return Compressed data vector
     * @throws std::runtime_error if compression fails
     */
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    
    /**
     * @brief Decompress ZSTD compressed data
     * 
     * @param compressedData Compressed data to decompress
     * @return Decompressed data vector
     * @throws std::runtime_error if decompression fails
     */
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData);
    
    /**
     * @brief Compress data with specified compression level
     * 
     * @param data Raw data to compress
     * @param level Compression level (1-22, higher = better compression but slower)
     * @return Compressed data vector
     * @throws std::runtime_error if compression fails
     */
    static std::vector<uint8_t> compressWithLevel(const std::vector<uint8_t>& data, int level);
    
    /**
     * @brief Calculate compression ratio
     * 
     * @param originalSize Size of original data in bytes
     * @param compressedSize Size of compressed data in bytes
     * @return Compression ratio as percentage (0.0 = no compression, 100.0 = complete compression)
     */
    static double getCompressionRatio(size_t originalSize, size_t compressedSize);
    
    /**
     * @brief Calculate compression ratio from data vectors
     * 
     * @param original Original data vector
     * @param compressed Compressed data vector
     * @return Compression ratio as percentage
     */
    static double getCompressionRatio(const std::vector<uint8_t>& original, 
                                    const std::vector<uint8_t>& compressed);
    
    /**
     * @brief Check if data would benefit from compression
     * 
     * @param data Data to evaluate
     * @param minSize Minimum size threshold for compression (default: 1024 bytes)
     * @return true if compression would be beneficial
     */
    static bool shouldCompress(const std::vector<uint8_t>& data, size_t minSize = 1024);
    
    /**
     * @brief Get ZSTD library version information
     * 
     * @return String containing ZSTD version
     */
    static std::string getVersion();

    // Statistics methods - always available
    static void resetStatistics();
    static void printSummary();
    static size_t getTotalCompressions();
    static size_t getTotalDecompressions();
    static size_t getTotalOriginalSize();
    static size_t getTotalCompressedSize();
    static int getCompressionLevel();

private:
    // Prevent instantiation - this is a utility class
    ZstdCompressor() = delete;
    ~ZstdCompressor() = delete;
    ZstdCompressor(const ZstdCompressor&) = delete;
    ZstdCompressor& operator=(const ZstdCompressor&) = delete;
    
    // Always track statistics
    static size_t totalCompressions;
    static size_t totalDecompressions;
    static size_t totalOriginalSize;
    static size_t totalCompressedSize;
    static int compressionLevel;
};

#endif // ZSTDCOMPRESSOR_HPP
