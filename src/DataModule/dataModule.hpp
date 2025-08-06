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
    StringBuffer stringBuffer;
    std::vector<std::unique_ptr<DataField>> metaDataFields;
    std::vector<std::vector<uint8_t>> metaDataRows;

    size_t metaDataRowSize = 0;
    
    // Schema reference resolution
    static std::unordered_map<std::string, nlohmann::json> schemaCache;
    nlohmann::json resolveSchemaReference(const std::string& refPath, const std::string& baseSchemaPath);

    explicit DataModule() {};
    DataModule(const std::string& schemaPath, UUID uuid, ModuleType type);

    void initialise();

    void parseSchemaHeader(const nlohmann::json& schemaJson);
    void parseSchema(const nlohmann::json& schemaJson);
    virtual void parseDataSchema(const nlohmann::json&) {};

    virtual std::unique_ptr<DataField> parseField(const std::string& name, 
                                            const nlohmann::json& definition,
                                            size_t& rowSize);

    std::ifstream openSchemaFile(const std::string& schemaPath);

    void writeMetaData(std::ostream& out);
    virtual std::streampos writeData(std::ostream& out) const = 0;
    void writeStringBuffer(std::ostream& out);
    void decodeMetadataRows(std::istream& in, size_t actualDataSize);
    virtual void decodeData(std::istream& in, size_t actualDataSize) = 0;

public:
    virtual ~DataModule() = default; 

    static std::unique_ptr<DataModule> fromStream(
        std::istream& in, uint64_t moduleStartOffset, uint64_t moduleSize, uint8_t moduleType);

    // static std::unique_ptr<DataModule> create(
    //     const std::string& schemaPath, UUID uuid, ModuleType type);

    const nlohmann::json& getSchema() const;
    //virtual void addMetaData(const nlohmann::json& rowData);
    void addMetaData(const nlohmann::json& rowData);
    // virtual void addData(const nlohmann::json& rowData) = 0;
    void writeBinary(std::ostream& out, XRefTable& xref);

    void printMetadata(std::ostream& out) const;
    virtual void printData(std::ostream& out) const = 0;
};


#endif