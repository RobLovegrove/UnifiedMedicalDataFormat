
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
        fields.emplace_back(parseField(name, definition));
    }
}

void TabularData::addData(const nlohmann::json& data) {
    addTableData(data, fields, rows);
}

void TabularData::writeData(ostream& out) const {
    uint64_t size = writeTableRows(out, rows);
    header->setDataSize(size);
}

void TabularData::readData(istream& in) {

    readTableRows(in, header->getDataSize(), fields, rows);
}

void TabularData::printData(std::ostream& out) const {

    printTableData(out, fields, rows);
}

std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
TabularData::getModuleSpecificData() const {
    return getTableDataAsJson(rows, fields);
}