#ifndef IMAGEDATA_HPP
#define IMAGEDATA_HPP

#include <nlohmann/json.hpp> 

#include "../stringBuffer.hpp"
#include "../dataModule.hpp"
#include "../Tabular/tabularData.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataField.hpp"
#include "../../Xref/xref.hpp"
#include "../../Utility/uuid.hpp"
#include "../../Utility/moduleType.hpp"

class ImageData : public TabularData { 

private:
    std::vector<uint8_t> rawImageData;

public:
    using TabularData::TabularData;

    virtual ~ImageData() override = default;
    explicit ImageData(const std::string& schemaPath, UUID uuid) 
    : TabularData(schemaPath, uuid, ModuleType::Image) {}

    void setImageData(const std::vector<uint8_t>& data) {
        rawImageData = data;
    }

    void writeBinary(std::ostream& out, XRefTable& xref) override;

    static std::unique_ptr<ImageData> fromStream(
        std::istream& in, uint64_t moduleStartOffset); 
};

#endif