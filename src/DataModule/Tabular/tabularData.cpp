
#include "tabularData.hpp"
#include "../Image/imageData.hpp"
#include "../../Utility/uuid.hpp"
#include "../../Xref/xref.hpp"
#include "../stringBuffer.hpp"
#include "../Header/dataHeader.hpp"
#include "../../Utility/Compression/ZstdCompressor.hpp"
#include "../../Utility/Encryption/EncryptionManager.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <memory>

using namespace std;

TabularData::TabularData(const string& schemaPath, UUID uuid, EncryptionData encryptionData) : DataModule(schemaPath, uuid, ModuleType::Tabular, encryptionData) {
    header->setDataCompression(CompressionType::ZSTD);
    initialise();
}

TabularData::TabularData(
    const string& schemaPath, const nlohmann::json& schemaJson, UUID uuid, EncryptionData encryptionData) 
    : DataModule(schemaPath, schemaJson, uuid, ModuleType::Tabular, encryptionData) {
    header->setDataCompression(CompressionType::ZSTD);
    initialise();
}

void TabularData::parseDataSchema(const nlohmann::json& schemaJson) {

    if (schemaJson.contains("required")) {
        dataRequired = schemaJson["required"];
    }

    for (const auto& field : dataRequired) {
        if (!schemaJson["properties"].contains(field)) {
            throw runtime_error("Schema must contain 'required' field: " + field);
        }
    }

    if (!schemaJson.contains("properties")) {
        throw runtime_error("Schema missing essential 'properties' field.");
    }

    const auto& props = schemaJson["properties"];

    for (const auto& [name, definition] : props.items()) {
        fields.emplace_back(parseField(name, definition));
    }
}

void TabularData::addData(const std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>>& data) {
    if (std::holds_alternative<nlohmann::json>(data)) {
        const auto& jsonData = std::get<nlohmann::json>(data);
        
        if (jsonData.is_array()) {
            // Handle array of data rows
            for (const auto& row : jsonData) {
                addTableData(row, fields, rows, dataRequired);
            }
        } else {
            // Handle single data row
            addTableData(jsonData, fields, rows, dataRequired);
        }
    }
}

void TabularData::writeData(ostream& out) const {

    if (header->getDataCompression() == CompressionType::ZSTD) {

        std::stringstream buffer;

        writeTableRows(buffer, rows);



        std::vector<uint8_t> dataBytes {
            std::istreambuf_iterator<char>(buffer),
            std::istreambuf_iterator<char>()
        };

        std::vector<uint8_t> compressedData = ZstdCompressor::compress(dataBytes);

        size_t compressedDataSize = compressedData.size();



        out.write(reinterpret_cast<const char*>(compressedData.data()), compressedDataSize);

        header->setDataSize(compressedDataSize);

    }
    else {
        uint64_t size = writeTableRows(out, rows);
        header->setDataSize(size);
    }


}

void TabularData::readData(istream& in) {

    readTableRows(in, header->getDataSize(), fields, rows);
}

void TabularData::printData(std::ostream& out) const {

    printTableData(out, fields, rows);
}

std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
TabularData::getModuleSpecificData() const {
    return getTableDataAsJson(dataRequired, rows, fields);
}