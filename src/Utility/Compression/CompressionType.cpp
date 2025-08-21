#include "CompressionType.hpp"

std::optional<CompressionType> stringToCompression(const std::string& str) {
    if (str == "RAW" || str == "raw") {
        return CompressionType::RAW;
    } else if (str == "JPEG2000_LOSSLESS" || str == "jpeg2000-lossless") {
        return CompressionType::JPEG2000_LOSSLESS;
    } else if (str == "PNG" || str == "png") {
        return CompressionType::PNG;
    } else if (str == "ZSTD" || str == "zstd") {
        return CompressionType::ZSTD;
    }
    return std::nullopt;
}

std::string compressionToString(CompressionType compression) {
    switch (compression) {
        case CompressionType::RAW:
            return "RAW";
        case CompressionType::JPEG2000_LOSSLESS:
            return "JPEG2000_LOSSLESS";
        case CompressionType::PNG:
            return "PNG";
        case CompressionType::ZSTD:
            return "ZSTD";
        default:
            return "UNKNOWN";
    }
}

CompressionType decodeCompressionType(const uint8_t& compression) {
    for (auto c : validCompressions) {
        if (static_cast<uint8_t>(c) == compression) {
            return c;
        }
    }
    return CompressionType::UNKNOWN;
}

uint8_t encodeCompression(const CompressionType& compression) {
    return static_cast<uint8_t>(compression);
}