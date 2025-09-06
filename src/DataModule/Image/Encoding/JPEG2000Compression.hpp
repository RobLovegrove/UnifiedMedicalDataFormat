#pragma once

#include "CompressionStrategy.hpp"
#include <openjpeg.h>

/**
 * @brief JPEG2000 compression strategy implementation
 */
class JPEG2000Compression : public CompressionStrategy {
private:
    // OpenJPEG memory stream data structure
    struct MemoryStreamData {
        const uint8_t* input_buffer;
        uint8_t* output_buffer;
        size_t input_size;
        size_t output_size;
        size_t current_pos;
        size_t max_output_size;
    };

    // OpenJPEG stream functions
    static OPJ_SIZE_T memory_read(void* buffer, OPJ_SIZE_T size, void* user_data);
    static OPJ_SIZE_T memory_write(void* buffer, OPJ_SIZE_T size, void* user_data);
    static OPJ_OFF_T memory_skip(OPJ_OFF_T offset, void* user_data);
    static OPJ_BOOL memory_seek(OPJ_OFF_T offset, void* user_data);

public:
    std::vector<uint8_t> compress(const std::vector<uint8_t>& rawData,
                                 int width, int height,
                                 uint8_t channels, uint8_t bitDepth) const override;
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData) const override;
    
    std::string getCompressionType() const override { return "JPEG2000_LOSSLESS"; }
    
    bool supports(int channels, uint8_t bitDepth) const override {
        // JPEG2000 supports 1-4 channels and various bit depths
        return channels >= 1 && channels <= 4 && bitDepth >= 8 && bitDepth <= 16;
    }
};
