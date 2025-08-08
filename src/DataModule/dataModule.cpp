#include "dataModule.hpp"
#include "./Image/imageData.hpp"
#include "./Tabular/tabularData.hpp"
#include "../Utility/uuid.hpp"
#include "../Xref/xref.hpp"
#include "stringBuffer.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <memory>

using namespace std;

DataModule::DataModule(const string& schemaPath, UUID uuid, ModuleType type) {

    header = make_unique<DataHeader>();
    header->setModuleType(type);
    header->setSchemaPath(schemaPath);
    header->setModuleID(uuid);

    ifstream file = openSchemaFile(schemaPath);
    file >> schemaJson;
}

void DataModule::initialise() {
    try {
        parseSchemaHeader(schemaJson);
        parseSchema(schemaJson);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse schema: " + std::string(e.what()));
    }
}

const nlohmann::json& DataModule::getSchema() const {
    return schemaJson;
}

void DataModule::parseSchema(const nlohmann::json& schemaJson) {

    const auto& props = schemaJson["properties"];

    if (!props.contains("metadata")) {
        throw std::runtime_error("Schema must contain 'metadata' in 'properties'");
    }

    // Parse metadata
    if (props.contains("metadata")) {
        const auto& metadataProps = props.at("metadata").at("properties");
        for (const auto& [name, definition] : metadataProps.items()) {
            metaDataFields.emplace_back(parseField(name, definition, metaDataRowSize));
        }
    }

    // Parse data
    if (props.contains("data")) {
        parseDataSchema(props.at("data"));
    }

}

void DataModule::parseSchemaHeader(const nlohmann::json& schemaJson) {
    vector<unique_ptr<DataField>> fields;

    if (!schemaJson.contains("properties")) {
        throw runtime_error("Schema missing essential 'properties' field.");
    }

    if (schemaJson.contains("endianness")) {
        string endian = schemaJson["endianness"];
        header->setLittleEndian(endian != "big");
    } else {
        header->setLittleEndian(true);  // Default to little-endian
    }

    string moduleType = schemaJson["module_type"];
    if (isValidModuleType(moduleType)) {
        header->setModuleType(module_type_from_string(moduleType));
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

unique_ptr<DataModule> DataModule::fromStream(
    istream& in, uint64_t moduleStartOffset, uint8_t moduleType) {

    unique_ptr<DataHeader> dmHeader = make_unique<DataHeader>();
    dmHeader->readDataHeader(in);
    
    unique_ptr<DataModule> dm;

    try {
        switch (static_cast<ModuleType>(moduleType)) {
        case ModuleType::Tabular:
            dm = make_unique<TabularData>(dmHeader->getSchemaPath(), dmHeader->getModuleID());
            break;
        case ModuleType::Image:
            dm = make_unique<ImageData>(dmHeader->getSchemaPath(), dmHeader->getModuleID());
            break;
        case ModuleType::Frame:
            dm = make_unique<FrameData>(dmHeader->getSchemaPath(), dmHeader->getModuleID());
            break;
        default:
            return nullptr;  // Gracefully skip unknown modules
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing module type " << static_cast<int>(moduleType)
                  << ": " << e.what() << std::endl;
        return nullptr;
    }

    dm->header = std::move(dmHeader);

    dm->header->setModuleStartOffset(moduleStartOffset);

    uint64_t relativeStringOffset = dm->header->getHeaderSize() + dm->header->getMetadataSize() + dm->header->getDataSize();
    uint64_t currentPos = in.tellg();
    
    in.seekg(relativeStringOffset);

    // Only read string buffer if there's actually content
    if (dm->header->getStringBufferSize() > 0) {
        dm->stringBuffer.readFromFile(in, dm->header->getStringBufferSize());
    }

    // Return back to start of data
    in.seekg(currentPos);

    // Read in metadata
    if (dm->header->getCompression()) {
        cout << "TODO: Handle file compression" << endl;
    } else {
        dm->decodeMetadataRows(in, dm->header->getMetadataSize());
    }

    // Read in data
    if (dm->header->getCompression()) {
        cout << "TODO: Handle file compression" << endl;
    } else {
        dm->decodeData(in, dm->header->getDataSize());
    }
    return dm;
}

void DataModule::writeMetaData(ostream& out) {
    
    for (const auto& row : metaDataRows) {
        out.write(reinterpret_cast<const char*>(row.data()), row.size());
    }
}

void DataModule::writeStringBuffer(ostream& out) {

    uint64_t stringBufferSize = stringBuffer.getSize();
    header->setStringBufferSize(stringBufferSize);

    if (stringBufferSize != 0) {
        stringBuffer.writeToFile(out);
    }
}

void DataModule::decodeMetadataRows(istream& in, size_t actualDataSize) {

    if (actualDataSize % metaDataRowSize != 0) {
        throw format_error("Data does not match row format");
    }

    size_t numRows = actualDataSize / metaDataRowSize;
    for (size_t i = 0; i < numRows; i++) {
        std::vector<uint8_t> row(metaDataRowSize);
        in.read(reinterpret_cast<char*>(row.data()), metaDataRowSize);

        if (in.gcount() != static_cast<std::streamsize>(metaDataRowSize)) {
            throw std::runtime_error("Failed to read full row from stream");
        }

        metaDataRows.push_back(std::move(row));
    }
}

unique_ptr<DataField> DataModule::parseField(const string& name,
                                                const nlohmann::json& definition,
                                                size_t& rowSize) {

    string type = definition.contains("type") ? definition["type"] : "string";

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

    // Handle numbers/floats
    else if (type == "number") {
        if (!definition.contains("format")) {
            throw runtime_error("Number field missing 'format': " + name);
        }

        std::string format = definition["format"];
        
        if (format == "float32" || format == "float64") {
            size_t length = (format == "float32") ? 4 : 8;
            rowSize += length;
            return make_unique<FloatField>(name, format);
        } else {
            throw runtime_error("Unsupported number format: " + format);
        }
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

    // Handle arrays
    else if (type == "array") {
        if (!definition.contains("items")) {
            throw runtime_error("Array field missing 'items': " + name);
        }
        if (!definition.contains("minItems") || !definition.contains("maxItems")) {
            throw runtime_error("Array field missing minItems/maxItems: " + name);
        }
        
        size_t minItems = definition["minItems"];
        size_t maxItems = definition["maxItems"];
        const auto& itemDef = definition["items"];
        
        size_t arraySize = 0;
        // Create a temporary field to calculate item size
        auto tempField = parseField("temp", itemDef, arraySize);
        arraySize = tempField->getLength() * maxItems;
        
        rowSize += arraySize;
        return make_unique<ArrayField>(name, itemDef, minItems, maxItems);
    }

    // Handle schema references ($ref)
    else if (definition.contains("$ref")) {
        std::string refPath = definition["$ref"];
        nlohmann::json resolvedDef = resolveSchemaReference(refPath, header->getSchemaPath());
        
        // Recursively parse the resolved definition
        return parseField(name, resolvedDef, rowSize);
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

void DataModule::writeBinary(std::ostream& out, XRefTable& xref) {
    
    streampos moduleStart = out.tellp();
    header->setModuleStartOffset(static_cast<uint64_t>(moduleStart));

    // Write header
    header->writeToFile(out);

    // Write Metadata
    header->setMetadataSize(metaDataRows.size() * metaDataRowSize);
    writeMetaData(out);

    // Write Data
    writeData(out);

    // Write String Buffer
    writeStringBuffer(out);

    streampos moduleEnd = out.tellp();

    header->setModuleSize(static_cast<uint64_t>(moduleEnd - moduleStart));

    // Update Header
    header->updateHeader(out);
    out.seekp(moduleEnd);

    // Update XrefTable
    xref.addEntry(
        header->getModuleType(), 
        header->getModuleID(), moduleStart, header->getModuleSize());

}

void DataModule::addMetaData(const nlohmann::json& data) {
    vector<uint8_t> row(metaDataRowSize, 0);
    size_t offset = 0;

    for (const unique_ptr<DataField>& field : metaDataFields) {
        const std::string& fieldName = field->getName();
        nlohmann::json value = data.contains(fieldName) ? data[fieldName] : nullptr;

        std::cout << "Adding: " << fieldName << ": " << value << std::endl;
        field->encodeToBuffer(value, row, offset);

        offset += field->getLength();
    }

    metaDataRows.push_back(std::move(row));
}

void DataModule::printMetadata(std::ostream& out) const {
    for (const auto& row : metaDataRows) {
        size_t offset = 0;
        nlohmann::json rowJson = nlohmann::json::object();

        for (const auto& field : metaDataFields) {
            rowJson[field->getName()] = field->decodeFromBuffer(row, offset);
            offset += field->getLength();
        }

        out << rowJson.dump(2) << "\n";
    }
}

// Template method that handles common functionality
ModuleData DataModule::getDataWithSchema() const {
    return {
        getMetadataAsJson(),
        getModuleSpecificData()
    };
}

// Helper method to reconstruct metadata from stored fields
nlohmann::json DataModule::getMetadataAsJson() const {
    nlohmann::json metadataArray = nlohmann::json::array();
    
    for (const auto& row : metaDataRows) {
        size_t offset = 0;
        nlohmann::json rowJson = nlohmann::json::object();

        for (const auto& field : metaDataFields) {
            rowJson[field->getName()] = field->decodeFromBuffer(row, offset);
            offset += field->getLength();
        }

        metadataArray.push_back(rowJson);
    }
    
    return metadataArray;
}

// Initialize static member
std::unordered_map<std::string, nlohmann::json> DataModule::schemaCache;

nlohmann::json DataModule::resolveSchemaReference(const std::string& refPath, const std::string& baseSchemaPath) {
    std::cout << "Resolving schema reference: " << refPath << std::endl;
    
    // Check if schema is already cached
    if (schemaCache.find(refPath) != schemaCache.end()) {
        std::cout << "Using cached schema for: " << refPath << std::endl;
        return schemaCache[refPath];
    }
    
    // Resolve relative path
    std::string fullPath = refPath;
    if (refPath.substr(0, 2) == "./") {
        // Get directory of base schema
        size_t lastSlash = baseSchemaPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string baseDir = baseSchemaPath.substr(0, lastSlash + 1);
            fullPath = baseDir + refPath.substr(2); // Remove "./" and prepend base directory
        }
    }
    
    std::cout << "Loading schema from: " << fullPath << std::endl;
    
    // Load the referenced schema
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open referenced schema file: " + fullPath);
    }
    
    nlohmann::json referencedSchema;
    file >> referencedSchema;
    
    std::cout << "Loaded schema: " << referencedSchema["$id"] << std::endl;
    
    // Cache the loaded schema
    schemaCache[refPath] = referencedSchema;
    
    return referencedSchema;
}