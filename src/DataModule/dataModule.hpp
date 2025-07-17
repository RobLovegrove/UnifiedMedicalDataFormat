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

class DataModule {
private:
    DataHeader header;
    nlohmann::json schemaJson;
    std::vector<std::unique_ptr<DataField>> fields;
    std::vector<std::vector<uint8_t>> rows;
    StringBuffer stringBuffer;
    size_t rowSize = 0;

    explicit DataModule() {};

    std::vector<std::unique_ptr<DataField>> parseSchema(const nlohmann::json& schemaJson);

    std::unique_ptr<DataField> parseField(const std::string& name, 
                                            const nlohmann::json& definition,
                                            size_t& rowSize);

    void decodeRows(std::istream& in, size_t actualDataSize);

public:
    explicit DataModule(const std::string& schemaPath, UUID uuid);
    
    const nlohmann::json& getSchema() const;
    void addRow(const nlohmann::json& rowData);
    void writeBinary(std::ostream& out, XRefTable& xref);

    void printRows(std::ostream& out) const;

    static std::unique_ptr<DataModule> fromStream(std::istream& in, uint64_t moduleStartOffset); 

};


#endif