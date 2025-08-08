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
#include <variant>

// Forward declaration for recursive structure
struct ModuleData;

struct ModuleData {
    nlohmann::json metadata;
    std::variant<
        nlohmann::json,                    // For tabular data
        std::vector<uint8_t>,              // For frame pixel data
        std::vector<ModuleData>            // For N-dimensional data
    > data;
};

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

    std::unique_ptr<DataField> parseField(const std::string& name, 
                                            const nlohmann::json& definition,
                                            size_t& rowSize);
                                            
    std::ifstream openSchemaFile(const std::string& schemaPath);

    void writeMetaData(std::ostream& out);
    virtual void writeData(std::ostream& out) const = 0;
    void writeStringBuffer(std::ostream& out);
    virtual void decodeMetadataRows(std::istream& in, size_t actualDataSize);
    virtual void decodeData(std::istream& in, size_t actualDataSize) = 0;

    // Helper method to reconstruct metadata from stored fields
    nlohmann::json getMetadataAsJson() const;

    // Getter for header (protected for derived classes)
    const DataHeader& getHeader() const { return *header; }

    // Virtual method for module-specific data
    virtual std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
    getModuleSpecificData() const = 0;


public:
    virtual ~DataModule() = default; 

    static std::unique_ptr<DataModule> fromStream(
        std::istream& in, uint64_t moduleStartOffset, uint8_t moduleType);

    // static std::unique_ptr<DataModule> create(
    //     const std::string& schemaPath, UUID uuid, ModuleType type);

    const nlohmann::json& getSchema() const;
    //virtual void addMetaData(const nlohmann::json& rowData);
    virtual void addMetaData(const nlohmann::json& rowData);
    // virtual void addData(const nlohmann::json& rowData) = 0;
    void writeBinary(std::ostream& out, XRefTable& xref);

    void printMetadata(std::ostream& out) const;
    virtual void printData(std::ostream& out) const = 0;

    // Template method that handles common functionality
    ModuleData getDataWithSchema() const;

    // Public methods to access header information
    UUID getModuleID() const { return header->getModuleID(); }
    ModuleType getModuleType() const { return header->getModuleType(); }
    std::string getSchemaPath() const { return header->getSchemaPath(); }
};


#endif