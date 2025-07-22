
#include "tabularData.hpp"
#include "../Image/imageData.hpp"
#include "../../Utility/uuid.hpp"
#include "../../Xref/xref.hpp"
#include "../stringBuffer.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <memory>

using namespace std;

TabularData::TabularData(const string& schemaPath, UUID uuid, ModuleType type) : DataModule(schemaPath, uuid) {

    header.moduleType = module_type_to_string(type);

    ifstream file = openSchemaFile(schemaPath);

    try {
        file >> schemaJson;
        parseSchemaHeader(schemaJson);
        parseSchema(schemaJson);

        for (unique_ptr<DataField>& f : fields) {
            cout << f;
        }
    } catch (const exception& e) {
        throw runtime_error("Failed to parse schema: " + string(e.what()));
    }
}

unique_ptr<TabularData> TabularData::fromStream(istream& in, uint64_t moduleStartOffset) {

    DataHeader dmHeader;
    dmHeader.readDataHeader(in);
    ModuleType type = module_type_from_string(dmHeader.moduleType);
    unique_ptr<TabularData> dm;
    if (type == ModuleType::Tabular) {
        dm = make_unique<TabularData>(dmHeader.schemaPath, dmHeader.moduleID, type);
    }
    else if (type == ModuleType::Image) {
        dm = make_unique<ImageData>(dmHeader.schemaPath, dmHeader.moduleID);
    }
    dm->header = dmHeader;

    dm->header.moduleStartOffset = moduleStartOffset;
    size_t actualDataSize = dm->header.stringOffset - (dm->header.moduleStartOffset + dm->header.headerSize);

    cout << dm->header << endl;

    uint64_t relativeStringOffset = dm->header.stringOffset - moduleStartOffset;
    uint64_t currentPos = in.tellg();
    
    in.seekg(relativeStringOffset);

    // Read in string buffer
    size_t stringBufferSize = dm->header.dataSize - dm->header.headerSize - actualDataSize;
    dm->stringBuffer.readFromFile(in, stringBufferSize);

    // Return back to start of data
    in.seekg(currentPos);

    // Read in data
    if (dm->header.compression) {
        cout << "TODO: Handle file compression" << endl;
    } else {
        dm->decodeRows(in, actualDataSize); // Data size is entire size including header size
    }

    return dm;
}

void TabularData::parseSchema(const nlohmann::json& schemaJson) {

    const auto& props = schemaJson["properties"];
    for (const auto& [name, definition] : props.items()) {
        fields.emplace_back(parseField(name, definition, rowSize));
    }

}

unique_ptr<DataField> TabularData::parseField(const string& name,
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


    // Handle integers
    else if (type == "integer") {
        if (!definition.contains("format")) {
            throw runtime_error("Integer field missing 'format': " + name);
        }

        std::string format = definition["format"];

        IntegerFormatInfo integerFormat = IntegerField::parseIntegerFormat(format);

        rowSize += integerFormat.byteLength;
        return make_unique<IntegerField>(name, integerFormat);
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

void TabularData::writeBinary(ostream& out, XRefTable& xref) {

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


void TabularData::decodeRows(istream& in, size_t actualDataSize) {

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

void TabularData::printRows(std::ostream& out) const {
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