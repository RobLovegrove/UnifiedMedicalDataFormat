#ifndef TABULARDATA_HPP
#define TABULARDATA_HPP

#include <nlohmann/json.hpp> 

#include "../stringBuffer.hpp"
#include "../dataModule.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataField.hpp"
#include "../../Xref/xref.hpp"
#include "../../Utility/uuid.hpp"

class TabularData : public DataModule { 

protected:
    std::vector<std::unique_ptr<DataField>> fields;
    std::vector<std::vector<uint8_t>> rows;
    
    size_t rowSize = 0;

    explicit TabularData() {};

    virtual void parseDataSchema(const nlohmann::json& schemaJson) override;

    std::unique_ptr<DataField> parseField(const std::string& name, 
                                            const nlohmann::json& definition,
                                            size_t& rowSize) override;

    void decodeData(std::istream& in, size_t actualDataSize) override;

    std::streampos writeData(std::ostream& out) const override;
    void writeStringBuffer(std::ostream& out);

public:
    explicit TabularData(const std::string& schemaPath, UUID uuid);
    virtual ~TabularData() override = default;
    
    void addData(const nlohmann::json& rowData);
    // void writeBinary(std::ostream& out, XRefTable& xref) override;

    void printData(std::ostream& out) const override;

};

#endif