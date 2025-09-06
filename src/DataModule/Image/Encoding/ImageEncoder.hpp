#ifndef IMAGEENCODER_HPP
#define IMAGEENCODER_HPP

#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <openjpeg.h>

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
    
    // Individual format methods
    std::vector<uint8_t> compressJPEG2000(const std::vector<uint8_t>& rawData, 
                                          int width, int height,
                                          uint8_t channels, uint8_t bitDepth) const;
    
    std::vector<uint8_t> decompressJPEG2000(const std::vector<uint8_t>& compressedData) const;
    
    std::vector<uint8_t> compressPNG(const std::vector<uint8_t>& rawData, 
                                     int width, int height,
                                     uint8_t channels, uint8_t bitDepth) const;
    
    std::vector<uint8_t> decompressPNG(const std::vector<uint8_t>& compressedData) const;
    
    // Strategy management methods
    void setCompressionStrategy(std::unique_ptr<CompressionStrategy> strategy);
    void setCompressionStrategy(const std::string& type);
    std::string getCurrentCompressionType() const;
    bool supports(int channels, uint8_t bitDepth) const;
    
    // Utility methods
    bool testOpenJPEGIntegration() const;
    bool testCompression() const;
    
private:
    // Compression strategy and factory
    std::unique_ptr<CompressionStrategy> compressionStrategy;
    std::shared_ptr<CompressionStrategyFactory> factory;
    
    // Helper method to convert CompressionType enum to string
    std::string compressionTypeToString(CompressionType type) const;
    
    // OpenJPEG memory stream data structure
    struct MemoryStreamData {
        const uint8_t* input_buffer;        // Input data for reading
        uint8_t* output_buffer;             // Output buffer for writing
        size_t input_size;                  // Size of input data
        size_t output_size;                 // Size of output data
        size_t current_pos;                 // Current position in stream
        size_t max_output_size;             // Maximum output buffer size
    };
    
    // OpenJPEG stream functions
    static OPJ_SIZE_T memory_read(void* buffer, OPJ_SIZE_T size, void* user_data);
    static OPJ_SIZE_T memory_write(void* buffer, OPJ_SIZE_T size, void* user_data);
    static OPJ_OFF_T memory_skip(OPJ_OFF_T offset, void* user_data);
    static OPJ_BOOL memory_seek(OPJ_OFF_T offset, void* user_data);
};

#endif // IMAGEENCODER_HPP 