#ifndef DATAMODULE_HPP
#define DATAMODULE_HPP

#include "dataField.hpp"
#include "Header/dataHeader.hpp"
#include "../Xref/xref.hpp"
#include "stringBuffer.hpp"

#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <memory>
#include <fstream>

class DataModule {
protected:
    std::unique_ptr<DataHeader> header;
    nlohmann::json schemaJson;

    void parseSchemaHeader(const nlohmann::json& schemaJson);
    virtual void parseSchema(const nlohmann::json& schemaJson) = 0;

    virtual std::unique_ptr<DataField> parseField(const std::string& name, 
                                            const nlohmann::json& definition,
                                            size_t& rowSize) = 0;

    std::ifstream openSchemaFile(const std::string& schemaPath);

public:
    virtual ~DataModule() = default; 
    explicit DataModule() {};
    const nlohmann::json& getSchema() const;
    virtual void addData(const nlohmann::json& rowData) = 0;
    virtual void writeBinary(std::ostream& out, XRefTable& xref) = 0;

    virtual void printData(std::ostream& out) const = 0;
};


#endif