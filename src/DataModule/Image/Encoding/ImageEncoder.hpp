#ifndef IMAGEENCODER_HPP
#define IMAGEENCODER_HPP

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

#include "../../../Utility/Compression/CompressionType.hpp"
#include "CompressionStrategy.hpp"
#include "CompressionFactory.hpp"

class ImageEncoder {
public:
    // Constructor
    ImageEncoder();
    
    // Constructor with dependency injection
    explicit ImageEncoder(std::unique_ptr<CompressionStrategy> strategy);
    
    // Destructor
    ~ImageEncoder() = default;
    
    // Main encoding/decoding interface
    std::vector<uint8_t> compress(const std::vector<uint8_t>& rawData, 
                                  CompressionType encoding,
                                  int width, int height,
                                  uint8_t channels, uint8_t bitDepth) const;
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData,
                                    CompressionType encoding) const;
    
    
    // Strategy management methods
    void setCompressionStrategy(std::unique_ptr<CompressionStrategy> strategy);
    void setCompressionStrategy(const std::string& type);
    std::string getCurrentCompressionType() const;
    bool supports(int channels, uint8_t bitDepth) const;
    
    // Utility methods
    bool testCompression() const;
    
private:
    // Compression strategy and factory
    std::unique_ptr<CompressionStrategy> compressionStrategy;
    std::shared_ptr<CompressionStrategyFactory> factory;
    
    // Helper method to convert CompressionType enum to string
    std::string compressionTypeToString(CompressionType type) const;
    
};

#endif // IMAGEENCODER_HPP 