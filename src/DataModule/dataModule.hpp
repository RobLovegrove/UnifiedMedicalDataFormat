#ifndef DATAMODULE_HPP
#define DATAMODULE_HPP

#include "dataField.hpp"
#include "dataHeader.hpp"
#include "../Xref/xref.hpp"
#include "stringBuffer.hpp"

#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <memory>
#include <fstream>

class DataModule {
protected:
    DataHeader header;
    nlohmann::json schemaJson;

    void parseSchemaHeader(const nlohmann::json& schemaJson);
    virtual void parseSchema(const nlohmann::json& schemaJson) = 0;

    virtual std::unique_ptr<DataField> parseField(const std::string& name, 
                                            const nlohmann::json& definition,
                                            size_t& rowSize) = 0;
    
    explicit DataModule() {};
    virtual ~DataModule() = default; 

    std::ifstream openSchemaFile(const std::string& schemaPath);

public:
    explicit DataModule(const std::string& schemaPath, UUID uuid);
    
    const nlohmann::json& getSchema() const;
    virtual void addData(const nlohmann::json& rowData) = 0;
    virtual void writeBinary(std::ostream& out, XRefTable& xref) = 0;

};


#endif