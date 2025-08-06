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

class ImageData : public DataModule { 

private:
    std::vector<uint8_t> rawImageData;

    std::streampos writeData(std::ostream& out) override;

public:

    virtual ~ImageData() override = default;
    explicit ImageData(const std::string& schemaPath, UUID uuid);
    void addData(const std::vector<uint8_t>& data) {
        rawImageData = data;
    }
    virtual void decodeData(std::istream& in, size_t actualDataSize) override;
    virtual void printData(std::ostream& out) const override;
};

#endif