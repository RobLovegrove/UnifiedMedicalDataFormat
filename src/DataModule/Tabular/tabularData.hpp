#ifndef TABULARDATA_HPP
#define TABULARDATA_HPP

#include <nlohmann/json.hpp> 

#include "../stringBuffer.hpp"
#include "../dataModule.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataField.hpp"
#include "../../Xref/xref.hpp"
#include "../../Utility/uuid.hpp"
#include "../ModuleData.hpp"
#include "../../Utility/Encryption/encryptionManager.hpp"

class TabularData : public DataModule { 

protected:
    std::vector<std::unique_ptr<DataField>> fields;
    std::vector<std::vector<uint8_t>> rows;
    
    size_t rowSize = 0;

    explicit TabularData() {};

    virtual void parseDataSchema(const nlohmann::json& schemaJson) override;

    void readData(std::istream& in) override;

    void writeData(std::ostream& out) const override;

    // Override the virtual method for tabular-specific data
    std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
    getModuleSpecificData() const override;

public:

    explicit TabularData(const std::string& schemaPath, DataHeader& dataheader);
    explicit TabularData(
        const std::string& schemaPath, const nlohmann::json& schemaJson, UUID uuid, EncryptionData encryptionData);
        
    virtual ~TabularData() override = default;
    
    virtual void addData(
        const std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>>&) override;
    // void writeBinary(std::ostream& out, XRefTable& xref) override;

    void printData(std::ostream& out) const override;

};

#endif