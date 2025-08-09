#ifndef IMAGEDATA_HPP
#define IMAGEDATA_HPP

#include <nlohmann/json.hpp> 
#include <vector>
#include <optional>
#include <string>

#include "../stringBuffer.hpp"
#include "../dataModule.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataField.hpp"
#include "../../Xref/xref.hpp"
#include "../../Utility/uuid.hpp"
#include "../../Utility/moduleType.hpp"
#include "FrameData.hpp"

// OpenJPEG includes
#include <openjpeg.h>

// Image encoding enum
enum class ImageEncoding {
    RAW,               // No compression
    JPEG2000_LOSSLESS, // JPEG 2000 lossless compression
    PNG                // PNG lossless compression
};

// Encoding conversion utilities
std::optional<ImageEncoding> stringToEncoding(const std::string& str);
std::string encodingToString(ImageEncoding encoding);

class ImageData : public DataModule { 

protected:
    std::vector<std::unique_ptr<DataField>> fields;
    std::vector<std::vector<uint8_t>> rows;
    
    size_t rowSize = 0;
    
    // Frame storage
    std::vector<std::unique_ptr<FrameData>> frames;
    
    // C++ dimensions array for efficient access
    std::vector<uint16_t> dimensions;
    
    // Image encoding
    ImageEncoding encoding;
    bool needsDecompression = false;
    
    // Image format attributes
    uint8_t bitDepth;
    uint8_t channels;

    explicit ImageData() {};

    virtual void parseDataSchema(const nlohmann::json& schemaJson) override;

    void decodeMetadataRows(std::istream& in, size_t actualDataSize) override;
    void decodeData(std::istream& in, size_t actualDataSize) override;

    void writeData(std::ostream& out) const override;
    void writeStringBuffer(std::ostream& out);

    // Override the virtual method for image-specific data
    std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
    getModuleSpecificData() const override;

public:
    explicit ImageData(const std::string& schemaPath, UUID uuid);
    virtual ~ImageData() override = default;
    
    void addData(std::unique_ptr<FrameData> frame);
    // void writeBinary(std::ostream& out, XRefTable& xref) override;

    void printData(std::ostream& out) const override;
    
    // Override addMetaData to populate dimensions array
    void addMetaData(const nlohmann::json& data) override;
    
    // Dimension access methods
    int getDepth() const;
    const std::vector<uint16_t>& getDimensions() const { return dimensions; }
    
    // Encoding methods
    void setEncoding(ImageEncoding enc);
    ImageEncoding getEncoding() const;
    std::string getEncodingString() const;
    bool setEncodingFromString(const std::string& enc_str);
    bool validateEncodingInSchema() const;
    
    // Image format getters
    uint8_t getBitDepth() const { return bitDepth; }
    uint8_t getChannels() const { return channels; }
    
    // OpenJPEG compression methods
    std::vector<uint8_t> compressJPEG2000(const std::vector<uint8_t>& rawData, 
                                          int width, int height) const;
    std::vector<uint8_t> decompressJPEG2000(const std::vector<uint8_t>& compressedData) const;
    std::vector<uint8_t> decompressFrameData(const std::vector<uint8_t>& compressedData) const;
    bool testOpenJPEGIntegration() const;

};

#endif