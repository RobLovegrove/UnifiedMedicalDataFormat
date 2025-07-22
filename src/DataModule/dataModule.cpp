#include "dataModule.hpp"
#include "../Utility/uuid.hpp"
#include "../Xref/xref.hpp"
#include "stringBuffer.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <memory>

using namespace std;

DataModule::DataModule(const string& schemaPath, UUID uuid) {
    header.schemaPath = schemaPath;
    header.moduleID = uuid;
}

const nlohmann::json& DataModule::getSchema() const {
    return schemaJson;
}


void DataModule::parseSchemaHeader(const nlohmann::json& schemaJson) {
    vector<unique_ptr<DataField>> fields;

    if (!schemaJson.contains("properties")) {
        throw runtime_error("Schema missing essential 'properties' field.");
    }

    if (schemaJson.contains("endianness")) {
        string endian = schemaJson["endianness"];
        header.littleEndian = (endian != "big");
    } else {
        header.littleEndian = true;  // Default to little-endian
    }

    string moduleType = schemaJson["module_type"];
    if (isValidModuleType(moduleType)) {
        header.moduleType = moduleType;
    }
    else {
        cout << "TODO: Handle what happens when a non valid moduleType is found" << endl;
    }
}

ifstream DataModule::openSchemaFile(const string& schemaPath) {
    ifstream file(schemaPath);
    if (!file.is_open()) {
        throw runtime_error("Failed to open schema file: " + schemaPath);
    }
    return file;
}