#include "CompressionFactory.hpp"
#include <functional>

CompressionFactory::CompressionFactory() {
    // Register built-in compression strategies
    registerStrategy("JPEG2000_LOSSLESS", []() -> std::unique_ptr<CompressionStrategy> {
        return std::make_unique<JPEG2000Compression>();
    });
    
    registerStrategy("PNG", []() -> std::unique_ptr<CompressionStrategy> {
        return std::make_unique<PNGCompression>();
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
