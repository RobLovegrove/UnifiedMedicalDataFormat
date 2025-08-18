#ifndef DATAMODULE_HPP
#define DATAMODULE_HPP

#include "dataField.hpp"
#include "Header/dataHeader.hpp"
#include "../Xref/xref.hpp"
#include "stringBuffer.hpp"
#include "ModuleData.hpp"

#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <memory>
#include <fstream>
#include <variant>

struct FieldInfo {
    size_t offset;   
    size_t length;   
    bool present;
    DataField* field;    
};

using FieldMap = std::unordered_map<std::string, FieldInfo>;

class DataModule {
protected:
    std::unique_ptr<DataHeader> header;
    nlohmann::json schemaJson;
    StringBuffer stringBuffer;
    std::vector<std::unique_ptr<DataField>> metaDataFields;
    std::vector<std::vector<uint8_t>> metaDataRows;
    std::vector<std::vector<std::unique_ptr<DataField>>> decodedMetaDataRows;

    std::vector<std::string> metadataRequired;
    std::vector<std::string> dataRequired;

    // Schema reference resolution
    static std::unordered_map<std::string, nlohmann::json> schemaCache;
    nlohmann::json resolveSchemaReference(const std::string& refPath, const std::string& baseSchemaPath);

    explicit DataModule() {};
    DataModule(const std::string& schemaPath, UUID uuid, ModuleType type);
    DataModule(
        const std::string& schemaPath, const nlohmann::json& schemaJson, UUID uuid, ModuleType type);

    void initialise();

    void parseSchemaHeader(const nlohmann::json& schemaJson);
    void parseSchema(const nlohmann::json& schemaJson);
    virtual void parseDataSchema(const nlohmann::json&) {};

    std::unique_ptr<DataField> parseField(const std::string& name, 
                                            const nlohmann::json& definition);
                                            
    std::ifstream openSchemaFile(const std::string& schemaPath);


    // Helper functions to avoid code duplication between Metadata and tabular data
    void addTableData(
        const nlohmann::json&, std::vector<std::unique_ptr<DataField>>&, 
        std::vector<std::vector<uint8_t>>&, std::vector<std::string>&);


    bool fieldExistsInData(const nlohmann::json& data, const std::string& fieldPath);
    nlohmann::json getNestedValue(const nlohmann::json& data, const std::string& fieldPath);


    size_t writeTableRows(std::ostream& out, const std::vector<std::vector<uint8_t>>& dataRows) const;

    void readTableRows
        (
            std::istream& in, 
            size_t dataSize, 
            std::vector<std::unique_ptr<DataField>>& fields, 
            std::vector<std::vector<uint8_t>>& rows
        );
    
    size_t readNestedObject(
        std::istream& in, 
        ObjectField* objectField, 
        std::vector<uint8_t>& row, 
        size_t writePos);

    void printTableData(
        std::ostream& out, const std::vector<std::unique_ptr<DataField>>& fields, const std::vector<std::vector<uint8_t>>& rows) const;
    
    nlohmann::json getTableDataAsJson(const std::vector<std::vector<uint8_t>>& rows, const std::vector<std::unique_ptr<DataField>>& fields) const;

    void writeMetaData(std::ostream& out);
    virtual void writeData(std::ostream& out) const = 0;
    void writeStringBuffer(std::ostream& out);
    virtual void readMetadataRows(std::istream& in);
    virtual void readData(std::istream& in) = 0;

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


    virtual void addData(const std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>>&) = 0;
    virtual void addMetaData(const nlohmann::json& rowData);

    void writeBinary(std::ostream& out, XRefTable& xref);

    void printMetadata(std::ostream& out) const;
    virtual void printData(std::ostream& out) const = 0;

    // Template method that handles common functionality
    ModuleData getDataWithSchema() const;

    // Public methods to access header information
    UUID getModuleID() const { return header->getModuleID(); }
    ModuleType getModuleType() const { return header->getModuleType(); }
    std::string getSchemaPath() const { return header->getSchemaPath(); }
    void setPrevious(uint64_t offset) { header->setPrevious(offset); }
};


#endif