#include "CompressionFactory.hpp"
#include <functional>
#include <iostream>

// Raw compression strategy (no compression)
class RawCompression : public CompressionStrategy {
public:
    std::vector<uint8_t> compress(const std::vector<uint8_t>& rawData,
                                 int width, int height,
                                 uint8_t channels, uint8_t bitDepth) const override {
        std::cerr << "RAW: No compression applied. Size: " << rawData.size() 
                  << " bytes, Dimensions: " << width << "x" << height 
                  << ", Channels: " << static_cast<int>(channels) 
                  << ", BitDepth: " << static_cast<int>(bitDepth) << std::endl;
        return rawData; // Return data as-is
    }
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData) const override {
        std::cerr << "RAW: No decompression needed. Size: " << compressedData.size() << " bytes" << std::endl;
        return compressedData; // Return data as-is
    }
    
    std::string getCompressionType() const override { 
        return "RAW"; 
    }
    
    bool supports([[maybe_unused]] int channels, [[maybe_unused]] uint8_t bitDepth) const override {
        return true; // RAW supports everything
    }
};

CompressionFactory::CompressionFactory() {
    // Register built-in compression strategies
    registerStrategy("JPEG2000_LOSSLESS", []() -> std::unique_ptr<CompressionStrategy> {
        return std::make_unique<JPEG2000Compression>();
    });
    
    registerStrategy("PNG", []() -> std::unique_ptr<CompressionStrategy> {
        return std::make_unique<PNGCompression>();
    });
    
    // Add RAW strategy (no compression)
    registerStrategy("RAW", []() -> std::unique_ptr<CompressionStrategy> {
        return std::make_unique<RawCompression>();
    });
}

std::unique_ptr<CompressionStrategy> CompressionFactory::createStrategy(const std::string& type) const {
    auto it = strategyCreators.find(type);
    if (it != strategyCreators.end()) {
        return it->second();
    }
    return nullptr;
}

std::vector<std::string> CompressionFactory::getSupportedTypes() const {
    std::vector<std::string> types;
    for (const auto& pair : strategyCreators) {
        types.push_back(pair.first);
    }
    return types;
}

void CompressionFactory::registerStrategy(const std::string& type, 
                                        std::function<std::unique_ptr<CompressionStrategy>()> creator) {
    strategyCreators[type] = creator;
}

bool CompressionFactory::isSupported(const std::string& type) const {
    return strategyCreators.find(type) != strategyCreators.end();
}
