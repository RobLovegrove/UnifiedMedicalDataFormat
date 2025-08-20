#ifndef COMPRESSIONTYPE_HPP
#define COMPRESSIONTYPE_HPP

#include <string>
#include <optional>
#include <array>

enum class CompressionType : uint8_t {
    UNKNOWN = 0,       // Unknown compression type  
    RAW,               // No compression
    JPEG2000_LOSSLESS, // JPEG 2000 lossless compression
    PNG,               // PNG lossless compression
    ZSTD,              // Zstandard compression
};

const std::array<CompressionType, 4> validCompressions = {
    CompressionType::RAW,
    CompressionType::JPEG2000_LOSSLESS,
    CompressionType::PNG,
    CompressionType::ZSTD
};


// Compression conversion utilities
std::optional<CompressionType> stringToCompression(const std::string& str);
std::string compressionToString(CompressionType compression);

CompressionType decodeCompressionType(const uint8_t& compression);

uint8_t encodeCompression(const CompressionType& compression);



#endif