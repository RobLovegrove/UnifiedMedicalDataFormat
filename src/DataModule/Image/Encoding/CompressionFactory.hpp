#pragma once

#include "CompressionStrategy.hpp"
#include "JPEG2000Compression.hpp"
#include "PNGCompression.hpp"
#include <map>
#include <memory>
#include <functional>

/**
 * @brief Factory for creating compression strategies
 * 
 * This factory follows the Factory pattern and provides a centralized way
 * to create compression strategies based on type identifiers.
 */
class CompressionFactory : public CompressionStrategyFactory {
private:
    std::map<std::string, std::function<std::unique_ptr<CompressionStrategy>()>> strategyCreators;

public:
    CompressionFactory();
    
    std::unique_ptr<CompressionStrategy> createStrategy(const std::string& type) const override;
    
    std::vector<std::string> getSupportedTypes() const override;
    
    /**
     * @brief Register a new compression strategy type
     * @param type Type identifier
     * @param creator Function to create the strategy
     */
    void registerStrategy(const std::string& type, 
                         std::function<std::unique_ptr<CompressionStrategy>()> creator);
    
    /**
     * @brief Check if a compression type is supported
     * @param type Type identifier
     * @return True if supported, false otherwise
     */
    bool isSupported(const std::string& type) const;
};
