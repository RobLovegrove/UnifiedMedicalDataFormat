#include "dataModule.hpp"
#include "../Utility/uuid.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <memory>

using namespace std;

DataModule::DataModule(const string& schemaPath) {
    header.schemaPath = schemaPath;

    ifstream file(schemaPath);
    if (!file.is_open()) {
        throw runtime_error("Failed to open schema file: " + schemaPath);
    }

    try {
        file >> schemaJson;
        fields = parseSchema(schemaJson);
        for (unique_ptr<DataField>& f : fields) {
            cout << f;
        }
    } catch (const exception& e) {
        throw runtime_error("Failed to parse schema: " + string(e.what()));
    }
}

const nlohmann::json& DataModule::getSchema() const {
    return schemaJson;
}

std::vector<std::unique_ptr<DataField>> DataModule::parseSchema(const nlohmann::json& schemaJson) {
    std::vector<std::unique_ptr<DataField>> fields;

    header.moduleID = UUID();

    if (!schemaJson.contains("properties")) {
        throw std::runtime_error("Schema missing essential 'properties' field.");
    }

    if (schemaJson.contains("endianness")) {
        std::string endian = schemaJson["endianness"];
        header.littleEndian = (endian != "big");
    } else {
        header.littleEndian = true;  // Default to little-endian
    }

    const auto& props = schemaJson["properties"];

    for (const auto& [name, definition] : props.items()) {
        std::string type = definition.value("type", "string");

        if (!definition.contains("length") && !definition.contains("storage")) {
            throw std::runtime_error("Missing 'length' or 'storage' for field: " + name);
        }


        // Handle enums
        if (definition.contains("enum")) {
            const auto& enumValues = definition["enum"];
            std::string storageType = "uint8";  // default

            if (definition.contains("storage") && definition["storage"].contains("type")) {
                storageType = definition["storage"]["type"];
            }
            size_t length = 1;
            if (storageType == "uint16") length = 2;
            else if (storageType == "uint32") length = 4;

            rowSize += length;
            fields.emplace_back(std::make_unique<EnumField>(name, enumValues, length));
        }

        // Handle strings
        else if (type == "string") {
            size_t length = definition["length"];
            rowSize += length;
            fields.emplace_back(std::make_unique<StringField>(name, type, length));
        }

        // Handle bools
        // else if (type == "bool") {
        //     size_t length = 1;
        //     rowSize += length;
        //     fields.emplace_back(std::make_unique<BoolField>(name));
        // }

        else {
            throw std::runtime_error("Unsupported type for field: " + name);
        }
    }

    return fields;
}


void DataModule::addRow(const unordered_map<string, string>& rowData) {
    vector<uint8_t> row(rowSize, 0);
    size_t offset = 0;

    for (const unique_ptr<DataField>& field : fields) {
        auto it = rowData.find(field->getName());
        string value = (it != rowData.end()) ? it->second : "";

        cout << field->getName() << ": " << value << endl;
        field->encodeToBuffer(value, row, offset);

        offset += field->getLength();
    }

    rows.push_back(std::move(row));
}

void DataModule::writeBinary(ostream& out) {

    streampos moduleStart = out.tellp();

    // Write header
    header.writeToFile(out);

    // Write Data
    for (const auto& row : rows) {
        out.write(reinterpret_cast<const char*>(row.data()), row.size());
    }

    streampos moduleEnd = out.tellp();


    uint32_t totalModuleSize = static_cast<uint32_t>(moduleEnd - moduleStart);
    header.writeDataSize(out, totalModuleSize);

    out.seekp(moduleEnd);
}