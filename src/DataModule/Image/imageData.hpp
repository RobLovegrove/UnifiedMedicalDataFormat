#ifndef IMAGEDATA_HPP
#define IMAGEDATA_HPP

#include <nlohmann/json.hpp> 
#include <vector>
#include <optional>
#include <string>
#include <utility>
#include <chrono>

#include "../stringBuffer.hpp"
#include "../dataModule.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataField.hpp"
#include "../../Xref/xref.hpp"
#include "../../Utility/uuid.hpp"
#include "../../Utility/moduleType.hpp"
#include "FrameData.hpp"
#include "ImageEncoder.hpp"
#include "../../Utility/Compression/CompressionType.hpp"


class ImageData : public DataModule { 

protected:
    // Frame storage
    std::vector<std::unique_ptr<FrameData>> frames;
    std::vector<uint16_t> dimensions;
    std::vector<std::string> dimensionNames;
    uint8_t bitDepth;
    uint8_t channels;
    
    // Image encoding
    bool needsDecompression = false;

    // Frame schema reference
    std::string frameSchemaPath;
    
    explicit ImageData() {};

    virtual void parseDataSchema(const nlohmann::json& schemaJson) override;

    void readMetadataRows(std::istream& in) override;
    void readData(std::istream& in) override;

    void writeData(std::ostream& out) const override;
    void writeStringBuffer(std::ostream& out);

    // Override the virtual method for image-specific data
    std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
    getModuleSpecificData() const override;

public:
    explicit ImageData(const std::string& schemaPath, DataHeader& dataheader);
    explicit ImageData(
        const std::string& schemaPath, const nlohmann::json& schemaJson, UUID uuid, EncryptionData encryptionData);

    virtual ~ImageData() override = default;
    
    virtual void addData(
        const std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>>&) override;

    void printData(std::ostream& out) const override;
    
    // Override addMetaData to populate dimensions array
    void addMetaData(const nlohmann::json& data) override;
    
    // Dimension access methods
    int getFrameCount() const;
    const std::vector<uint16_t>& getDimensions() const { return dimensions; }
    
    // Get only non-zero dimensions
    std::vector<uint16_t> getNonZeroDimensions() const;
    
    // Get dimension names for non-zero dimensions
    std::vector<std::string> getNonZeroDimensionNames() const;
    
    // Encoding methods
    void setEncoding(CompressionType enc);
    CompressionType getEncoding() const;
    bool validateEncodingInSchema() const;
    
    // Image format getters
    uint8_t getBitDepth() const { return bitDepth; }
    uint8_t getChannels() const { return channels; }
    
    // Image encoder for compression/decompression
    std::unique_ptr<ImageEncoder> encoder;
    
    // Frame schema access
    const std::string& getFrameSchemaPath() const { return frameSchemaPath; }
    
    // Decompression helper method
    std::vector<uint8_t> decompressFrameData(const std::vector<uint8_t>& compressedData) const;

};

#endif