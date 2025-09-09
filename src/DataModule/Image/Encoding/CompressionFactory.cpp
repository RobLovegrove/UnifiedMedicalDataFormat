#include "CompressionFactory.hpp"
#include <functional>
#include <iostream>

// Raw compression strategy (no compression)
class RawCompression : public CompressionStrategy {
public:
    std::vector<uint8_t> compress(const std::vector<uint8_t>& rawData,
                                 int, int,
                                 uint8_t, uint8_t) const override {
        return rawData; // Return data as-is
    }
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData) const override {
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
    registerStrategy(CompressionType::JPEG2000_LOSSLESS, []() -> std::unique_ptr<CompressionStrategy> {
        return std::make_unique<JPEG2000Compression>();
    });
    
    registerStrategy(CompressionType::PNG, []() -> std::unique_ptr<CompressionStrategy> {
        return std::make_unique<PNGCompression>();
    });
    
    // Add RAW strategy (no compression)
    registerStrategy(CompressionType::RAW, []() -> std::unique_ptr<CompressionStrategy> {
        return std::make_unique<RawCompression>();
    });
}

std::unique_ptr<CompressionStrategy> CompressionFactory::createStrategy(CompressionType type) const {
    auto it = strategyCreators.find(type);
    if (it != strategyCreators.end()) {
        return it->second();
    }
    return nullptr;
}

std::vector<CompressionType> CompressionFactory::getSupportedTypes() const {
    std::vector<CompressionType> types;
    for (const auto& pair : strategyCreators) {
        types.push_back(pair.first);
    }
    return types;
}

void CompressionFactory::registerStrategy(CompressionType type, 
                                        std::function<std::unique_ptr<CompressionStrategy>()> creator) {
    strategyCreators[type] = creator;
}

bool CompressionFactory::isSupported(CompressionType type) const {
    return strategyCreators.find(type) != strategyCreators.end();
}
