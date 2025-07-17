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
    header.moduleType = module_type_to_string(ModuleType::Tabular);

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

unique_ptr<DataModule> DataModule::fromStream(istream& in, uint64_t moduleStartOffset) {


    DataHeader dmHeader;
    dmHeader.readDataHeader(in);
    unique_ptr<DataModule> dm = make_unique<DataModule>(dmHeader.schemaPath, dmHeader.moduleID);
    dm->header = dmHeader;

    dm->header.moduleStartOffset = moduleStartOffset;
    size_t actualDataSize = dm->header.stringOffset - (dm->header.moduleStartOffset + dm->header.headerSize);

    cout << dm->header << endl;

    if (dm->header.compression) {
        cout << "TODO: Handle file compression" << endl;
    } else {
        dm->decodeRows(in, actualDataSize); // Data size is entire size including header size
    }

    // Read in string buffer
    size_t stringBufferSize = dm->header.dataSize - dm->header.headerSize - actualDataSize;
    dm->stringBuffer.readFromFile(in, stringBufferSize);

    return dm;
}

const nlohmann::json& DataModule::getSchema() const {
    return schemaJson;
}


vector<unique_ptr<DataField>> DataModule::parseSchema(const nlohmann::json& schemaJson) {
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

    const auto& props = schemaJson["properties"];

    for (const auto& [name, definition] : props.items()) {
        fields.emplace_back(parseField(name, definition, rowSize));
    }

    return fields;
}

unique_ptr<DataField> DataModule::parseField(const string& name,
                                                const nlohmann::json& definition,
                                                size_t& rowSize) {

    string type = definition.value("type", "string");

    // Handle enums
    if (definition.contains("enum")) {
        const auto& enumValues = definition["enum"];
        string storageType = "uint8";  // default

        if (definition.contains("storage") && definition["storage"].contains("type")) {
            storageType = definition["storage"]["type"];
        }
        size_t length = 1;
        if (storageType == "uint16") length = 2;
        else if (storageType == "uint32") length = 4;

        rowSize += length;
        return make_unique<EnumField>(name, enumValues, length);
    }

    // Handle strings
    else if (type == "string") {

        // Handle fixed length strings
        if (definition.contains("length")) {
            size_t length = definition["length"];
            rowSize += length;
            return make_unique<StringField>(name, type, length);
        }
        else {
            // Add size of stringStartOffset and size of stringLength to row
            rowSize += (sizeof(uint64_t) + sizeof(uint32_t));
            return make_unique<VarStringField>(name, &stringBuffer);
        }  
    }

    // Handle objects recursively
    else if (type == "object") {
        if (!definition.contains("properties")) {
            throw runtime_error("Object field missing 'properties': " + name);
        }

        vector<unique_ptr<DataField>> subfields;
        size_t objectSize = 0;

        for (const auto& [subname, subdef] : definition["properties"].items()) {
            string fullSubName = subname;
            subfields.emplace_back(parseField(fullSubName, subdef, objectSize));
        }

        rowSize += objectSize;
        return make_unique<ObjectField>(name, std::move(subfields), objectSize);
    }

    // Handle bools
    // else if (type == "bool") {
    //     size_t length = 1;
    //     rowSize += length;
    //     fields.emplace_back(std::make_unique<BoolField>(name));
    // }

    // Handle other types later (e.g., bool, arrays)
    else {
        throw runtime_error("Unsupported type for field: " + name);
    }
    
}

void DataModule::addRow(const nlohmann::json& rowData) {
    vector<uint8_t> row(rowSize, 0);
    size_t offset = 0;

    for (const unique_ptr<DataField>& field : fields) {
        const std::string& fieldName = field->getName();
        nlohmann::json value = rowData.contains(fieldName) ? rowData[fieldName] : nullptr;

        std::cout << "Adding: "  << fieldName << ": " << value << std::endl;

        field->encodeToBuffer(value, row, offset);

        offset += field->getLength();
    }

    rows.push_back(std::move(row));
}

void DataModule::writeBinary(ostream& out, XRefTable& xref) {

    streampos moduleStart = out.tellp();

    // Write header
    header.writeToFile(out);

    // Write Data
    for (const auto& row : rows) {
        out.write(reinterpret_cast<const char*>(row.data()), row.size());
    }

    streampos stringBufferStart = out.tellp();

    // Write String Buffer
    if (stringBuffer.getSize() != 0) {
        stringBuffer.writeToFile(out);
    }

    streampos moduleEnd = out.tellp();

    uint32_t totalModuleSize = static_cast<uint32_t>(moduleEnd - moduleStart);
    header.updateHeader(out, totalModuleSize, static_cast<uint64_t>(stringBufferStart));

    out.seekp(moduleEnd);

    xref.addEntry(ModuleType::Tabular, header.moduleID, moduleStart, totalModuleSize);
}


void DataModule::decodeRows(istream& in, size_t actualDataSize) {

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

void DataModule::printRows(std::ostream& out) const {
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