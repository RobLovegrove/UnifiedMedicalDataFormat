#ifndef FRAMEDATA_HPP
#define FRAMEDATA_HPP

#include "../dataModule.hpp"

#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <ostream>
#include <istream>

class FrameData : public DataModule {
    friend class ImageData; // Allow ImageData to access protected members
    
public:
    mutable std::vector<uint8_t> pixelData; // Allow modification in const methods
    mutable bool needsDecompression = false; // Track if data needs decompression

    FrameData(const std::string& schemaPath, UUID uuid, EncryptionData encryptionData);
    
    // No copy/move operations supported due to DataModule design

    virtual void addData(
        const std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>>&) override;
    virtual void writeData(std::ostream& out) const override;
    virtual void readData(std::istream& in) override;
    virtual void printData(std::ostream& out) const override;
    
    // Override the virtual method for frame-specific data
    std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
    getModuleSpecificData() const override;
    
    // Metadata methods
    size_t getMetadataSize() const;
};

#endif // FRAMEDATA_HPP