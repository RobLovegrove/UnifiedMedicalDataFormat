#ifndef UNKNOWN_DATA_HPP
#define UNKNOWN_DATA_HPP

#include <nlohmann/json.hpp> 

#include "../stringBuffer.hpp"
#include "../dataModule.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataField.hpp"
#include "../../Xref/xref.hpp"
#include "../../Utility/uuid.hpp"
#include "../ModuleData.hpp"
#include "../../Utility/Encryption/encryptionManager.hpp"

class UnknownData : public DataModule { 

protected:
    std::vector<std::unique_ptr<DataField>> fields;
    std::vector<std::vector<uint8_t>> rows;
    
    size_t rowSize = 0;

    explicit UnknownData() {};

    virtual void parseDataSchema(const nlohmann::json&) override {}

    void readData(std::istream&) override {}

    void writeData(std::ostream&) const override {}

    // Override the virtual method for tabular-specific data
    std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
    getModuleSpecificData() const override;

public:

    explicit UnknownData(const std::string& schemaPath, DataHeader& dataheader);
    explicit UnknownData(
        const std::string& schemaPath, const nlohmann::json& schemaJson, UUID uuid, EncryptionData encryptionData);
        
    virtual ~UnknownData() override = default;
    
    virtual void addData(
        const std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>>&) override {
            throw std::runtime_error("UnknownData does not support adding data");
        }

};

#endif