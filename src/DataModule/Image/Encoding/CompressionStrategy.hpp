#pragma once

#include <vector>
#include <memory>
#include <string>

/**
 * @brief Abstract interface for image compression strategies
 * 
 * This interface follows the Strategy pattern and Dependency Inversion Principle,
 * allowing ImageEncoder to depend on abstractions rather than concrete implementations.
 */
class CompressionStrategy {
public:
    virtual ~CompressionStrategy() = default;
    
    /**
     * @brief Compress raw image data
     * @param rawData Raw pixel data
     * @param width Image width
     * @param height Image height
     * @param channels Number of color channels
     * @param bitDepth Bits per channel
     * @return Compressed data
     */
    virtual std::vector<uint8_t> compress(const std::vector<uint8_t>& rawData,
                                         int width, int height,
                                         uint8_t channels, uint8_t bitDepth) const = 0;
    
    /**
     * @brief Decompress image data
     * @param compressedData Compressed data
     * @return Decompressed raw data
     */
    virtual std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData) const = 0;
    
    /**
     * @brief Get the compression type identifier
     * @return String identifier for this compression type
     */
    virtual std::string getCompressionType() const = 0;
    
    /**
     * @brief Check if this strategy supports the given parameters
     * @param channels Number of color channels
     * @param bitDepth Bits per channel
     * @return True if supported, false otherwise
     */
    virtual bool supports(int channels, uint8_t bitDepth) const = 0;
};

/**
 * @brief Factory interface for creating compression strategies
 */
class CompressionStrategyFactory {
public:
    virtual ~CompressionStrategyFactory() = default;
    
    /**
     * @brief Create a compression strategy for the given type
     * @param type Compression type identifier
     * @return Unique pointer to compression strategy, or nullptr if unsupported
     */
    virtual std::unique_ptr<CompressionStrategy> createStrategy(const std::string& type) const = 0;
    
    /**
     * @brief Get list of supported compression types
     * @return Vector of supported compression type strings
     */
    virtual std::vector<std::string> getSupportedTypes() const = 0;
};
