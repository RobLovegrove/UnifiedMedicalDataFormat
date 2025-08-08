
#include "tabularData.hpp"
#include "../Image/imageData.hpp"
#include "../../Utility/uuid.hpp"
#include "../../Xref/xref.hpp"
#include "../stringBuffer.hpp"
#include "../Header/dataHeader.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <memory>

using namespace std;

TabularData::TabularData(const string& schemaPath, UUID uuid) : DataModule(schemaPath, uuid, ModuleType::Tabular) {
    initialise();
}

void TabularData::parseDataSchema(const nlohmann::json& schemaJson) {

    if (!schemaJson.contains("properties")) {
        throw runtime_error("Schema missing essential 'properties' field.");
    }

    const auto& props = schemaJson["properties"];

    for (const auto& [name, definition] : props.items()) {
        fields.emplace_back(parseField(name, definition, rowSize));
    }

}

void TabularData::addData(const nlohmann::json& data) {
    vector<uint8_t> row(rowSize, 0);
    size_t offset = 0;

    for (const unique_ptr<DataField>& field : fields) {
        const std::string& fieldName = field->getName();
        nlohmann::json value = data.contains(fieldName) ? data[fieldName] : nullptr;

        std::cout << "Adding: "  << fieldName << ": " << value << std::endl;

        field->encodeToBuffer(value, row, offset);

        offset += field->getLength();
    }

    rows.push_back(std::move(row));
}

void TabularData::writeData(ostream& out) const {

    header->setDataSize(rows.size() * rowSize);
    
    for (const auto& row : rows) {
        out.write(reinterpret_cast<const char*>(row.data()), row.size());
    }
}

void TabularData::writeStringBuffer(ostream& out) {

    if (stringBuffer.getSize() != 0) {
        stringBuffer.writeToFile(out);
    }
}

void TabularData::decodeData(istream& in, size_t actualDataSize) {

    if (actualDataSize % rowSize != 0) {
        throw format_error("Data does not match row format");
    }

    size_t numRows = actualDataSize / rowSize;
    for (size_t i = 0; i < numRows; i++) {
        std::vector<uint8_t> row(rowSize);
        in.read(reinterpret_cast<char*>(row.data()), rowSize);

        if (in.gcount() != static_cast<std::streamsize>(rowSize)) {
            throw std::runtime_error("Failed to read full row from stream");
        }

        rows.push_back(std::move(row));
    }
}

void TabularData::printData(std::ostream& out) const {
    for (const auto& row : rows) {
        size_t offset = 0;
        nlohmann::json rowJson = nlohmann::json::object();

        for (const auto& field : fields) {
            rowJson[field->getName()] = field->decodeFromBuffer(row, offset);
            offset += field->getLength();
        }

        out << rowJson.dump(2) << "\n";  // Pretty-print with 2-space indentation
    }
}

// Override the virtual method for tabular-specific data
std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
TabularData::getModuleSpecificData() const {
    nlohmann::json dataArray = nlohmann::json::array();
    
    for (const auto& row : rows) {
        size_t offset = 0;
        nlohmann::json rowJson = nlohmann::json::object();

        for (const auto& field : fields) {
            rowJson[field->getName()] = field->decodeFromBuffer(row, offset);
            offset += field->getLength();
        }

        dataArray.push_back(rowJson);
    }
    
    return dataArray;  // Returns JSON variant
}