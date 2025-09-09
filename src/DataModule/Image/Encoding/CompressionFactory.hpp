#pragma once

#include "CompressionStrategy.hpp"
#include "JPEG2000Compression.hpp"
#include "PNGCompression.hpp"
#include "Utility/Compression/CompressionType.hpp"
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
    std::map<CompressionType, std::function<std::unique_ptr<CompressionStrategy>()>> strategyCreators;

public:
    CompressionFactory();
    
    std::unique_ptr<CompressionStrategy> createStrategy(CompressionType type) const override;
    
    std::vector<CompressionType> getSupportedTypes() const override;
    
    /**
     * @brief Register a new compression strategy type
     * @param type Compression type enum
     * @param creator Function to create the strategy
     */
    void registerStrategy(CompressionType type, 
                         std::function<std::unique_ptr<CompressionStrategy>()> creator);
    
    /**
     * @brief Check if a compression type is supported
     * @param type Compression type enum
     * @return True if supported, false otherwise
     */
    bool isSupported(CompressionType type) const;
};
