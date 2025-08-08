#ifndef IMAGEDATA_HPP
#define IMAGEDATA_HPP

#include <nlohmann/json.hpp> 
#include <vector>

#include "../stringBuffer.hpp"
#include "../dataModule.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataField.hpp"
#include "../../Xref/xref.hpp"
#include "../../Utility/uuid.hpp"
#include "../../Utility/moduleType.hpp"
#include "FrameData.hpp"

class ImageData : public DataModule { 

protected:
    std::vector<std::unique_ptr<DataField>> fields;
    std::vector<std::vector<uint8_t>> rows;
    
    size_t rowSize = 0;
    
    // Frame storage
    std::vector<std::unique_ptr<FrameData>> frames;
    
    // C++ dimensions array for efficient access
    std::vector<uint16_t> dimensions;

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

};

#endif