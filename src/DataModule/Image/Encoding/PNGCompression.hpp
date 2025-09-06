#pragma once

#include "CompressionStrategy.hpp"
#include <png.h>

/**
 * @brief PNG compression strategy implementation
 */
class PNGCompression : public CompressionStrategy {
public:
    std::vector<uint8_t> compress(const std::vector<uint8_t>& rawData,
                                 int width, int height,
                                 uint8_t channels, uint8_t bitDepth) const override;
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData) const override;
    
    std::string getCompressionType() const override { return "PNG"; }
    
    bool supports(int channels, uint8_t bitDepth) const override {
        // PNG supports 1, 3, or 4 channels and 8 or 16 bit depth
        return (channels == 1 || channels == 3 || channels == 4) && 
               (bitDepth == 8 || bitDepth == 16);
    }
};
