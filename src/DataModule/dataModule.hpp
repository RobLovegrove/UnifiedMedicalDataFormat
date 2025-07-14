#ifndef DATAMODULE_HPP
#define DATAMODULE_HPP

#include "dataField.hpp"
#include "dataHeader.hpp"

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
    size_t rowSize = 0;

    std::vector<std::unique_ptr<DataField>> parseSchema(const nlohmann::json& schemaJson);
    

public:
    explicit DataModule(const std::string& schemaPath);
    
    const nlohmann::json& getSchema() const;
    void addRow(const std::unordered_map<std::string, std::string>& rowData);

    void writeBinary(std::ostream& out);

};


#endif