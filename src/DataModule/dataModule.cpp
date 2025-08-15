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
#include <vector>

using namespace std;

DataModule::DataModule(const string& schemaPath, UUID uuid, ModuleType type) {

    header = make_unique<DataHeader>();
    header->setModuleType(type);
    header->setSchemaPath(schemaPath);
    header->setModuleID(uuid);

    ifstream file = openSchemaFile(schemaPath);
    file >> schemaJson;

    if (type == ModuleType::Image) { cout << schemaJson.dump(2) << endl; }
    
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
            metaDataFields.emplace_back(parseField(name, definition));
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

    cout << "Reading data header" << endl;
    dmHeader->readDataHeader(in);
    cout << "Done reading data header" << endl;

    if (static_cast<ModuleType>(moduleType) != ModuleType::Frame) {
        cout << *dmHeader << endl;
    }
    
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

    // Only read string buffer if there's actually content
    if (dm->header->getStringBufferSize() > 0) {
        dm->stringBuffer.readFromFile(in, dm->header->getStringBufferSize());
    }

    // Read in metadata
    if (dm->header->getMetadataSize() > 0) {
    if (dm->header->getCompression()) {
        cout << "TODO: Handle file compression" << endl;
    } else {
            std::vector<uint8_t> buffer(dm->header->getMetadataSize());
            in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

            std::istringstream inputStream(
                std::string(reinterpret_cast<char*>(buffer.data()), buffer.size())
            );

            if (in.gcount() != static_cast<std::streamsize>(buffer.size())) {
                throw std::runtime_error("Failed to read full metadata block");
            }

            dm->readMetadataRows(inputStream);
        }
    }

    if (dm->header->getDataSize() > 0) {
    // Read in data
    if (dm->header->getCompression()) {
        cout << "TODO: Handle file compression" << endl;
    } else {
    
            std::vector<uint8_t> buffer(dm->header->getDataSize());
    
            in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    
            std::istringstream inputStream(
                std::string(reinterpret_cast<char*>(buffer.data()), buffer.size())
            );
    
            if (in.gcount() != static_cast<std::streamsize>(buffer.size())) {
                throw std::runtime_error("Failed to read full data block");
            }        
            dm->readData(inputStream);
        }
    }
    return dm;
}

void DataModule::writeMetaData(ostream& out) {
    
    uint32_t size = writeTableRows(out, metaDataRows);
    header->setMetadataSize(size);
}

size_t DataModule::writeTableRows(ostream& out, const vector<std::vector<uint8_t>>& dataRows) const {
    size_t totalSize = 0;
    for (size_t i = 0; i < dataRows.size(); ++i) {
        const auto& row = dataRows[i];
        out.write(reinterpret_cast<const char*>(row.data()), row.size());
        totalSize += row.size();
    }
    return totalSize;
}

void DataModule::writeStringBuffer(ostream& out) {

    uint64_t stringBufferSize = stringBuffer.getSize();
    header->setStringBufferSize(stringBufferSize);

    if (stringBufferSize != 0) {
        stringBuffer.writeToFile(out);
    }
}

size_t calcRowSize(const std::vector<uint8_t>& bitmap,
                   const std::vector<std::unique_ptr<DataField>>& fields) {
    size_t bitmapSize = (fields.size() + 7) / 8;
    size_t size = bitmapSize;
    for (size_t i = 0; i < fields.size(); ++i) {
        if (bitmap[i / 8] & (uint8_t(1) << (i % 8))) {
            size += fields[i]->getLength();
        }
    }
    return size;
}

void DataModule::readTableRows(
    istream& in, size_t dataSize, vector<unique_ptr<DataField>>& fields, 
    vector<std::vector<uint8_t>>& rows) {

    // Build flattened field list including nested fields
    std::vector<std::pair<std::string, DataField*>> flattenedFields;
    
    for (const auto& field : fields) {
        if (auto* objectField = dynamic_cast<ObjectField*>(field.get())) {
            // Add nested fields with dot notation
            const auto& nestedFields = objectField->getNestedFields();
            for (const auto& nestedField : nestedFields) {
                std::string fieldPath = field->getName() + "." + nestedField->getName();
                flattenedFields.push_back({fieldPath, nestedField.get()});
            }
        } else {
            // Regular field
            flattenedFields.push_back({field->getName(), field.get()});
        }
    }
    
    size_t numFlattenedFields = flattenedFields.size();
    size_t bitmapSize = (numFlattenedFields + 7) / 8;

    cout << "The number of fields is: " << numFlattenedFields << endl;
    cout << "Bitmap size: " << bitmapSize << " bytes" << endl;
    
    // Debug: Print flattened field names
    cout << "Flattened fields:" << endl;
    for (size_t i = 0; i < flattenedFields.size(); ++i) {
        const auto& [fieldPath, fieldPtr] = flattenedFields[i];
        cout << "  " << i << ": " << fieldPath << " (type: " << fieldPtr->getType() << ")" << endl;
    }

    size_t bytesRemaining = dataSize;

    while (bytesRemaining > 0) {
        // Read bitmap for all fields (including nested)
        std::vector<uint8_t> bitmap(bitmapSize);
        in.read(reinterpret_cast<char*>(bitmap.data()), bitmapSize);
        if (in.gcount() != static_cast<std::streamsize>(bitmapSize)) {
            throw std::runtime_error("Truncated bitmap");
        }
        
        // Enhanced debug: Print bitmap and field presence (only for main module)
        if (numFlattenedFields > 10) {  // Main image module has 15 fields, frames have 7
            cout << "=== READING ROW DEBUG (MAIN MODULE) ===" << endl;
            cout << "Bitmap bytes: ";
            for (size_t i = 0; i < bitmapSize; ++i) {
                cout << std::hex << static_cast<int>(bitmap[i]) << " ";
            }
            cout << std::dec << endl;
            
            cout << "Bitmap bits: ";
            for (size_t i = 0; i < numFlattenedFields; ++i) {
                bool present = (bitmap[i / 8] & (1 << (i % 8))) != 0;
                cout << (present ? "1" : "0");
            }
            cout << endl;
            
            cout << "Field presence:" << endl;
            for (size_t i = 0; i < numFlattenedFields; ++i) {
                bool present = (bitmap[i / 8] & (1 << (i % 8))) != 0;
                const auto& [fieldPath, fieldPtr] = flattenedFields[i];
                cout << "  " << i << ": " << fieldPath << " - " << (present ? "PRESENT" : "MISSING") << " (length: " << fieldPtr->getLength() << ")" << endl;
            }
        }
        
        // Calculate row size based on which fields are present
        size_t rowSize = bitmapSize;
        for (size_t i = 0; i < numFlattenedFields; ++i) {
            bool present = bitmap[i / 8] & (1 << (i % 8));
            if (present) {
                const auto& [fieldPath, fieldPtr] = flattenedFields[i];
                rowSize += fieldPtr->getLength();
            }
        }
        
        if (numFlattenedFields > 10) {  // Main module only
            cout << "Calculated row size: " << rowSize << " bytes (bitmap: " << bitmapSize << " + data: " << (rowSize - bitmapSize) << ")" << endl;
        }
        
        // Start a row with the bitmap
        std::vector<uint8_t> row(rowSize);

        // Start writing at position 0
        size_t writePos = 0;
        memcpy(row.data(), bitmap.data(), bitmapSize);
        writePos += bitmapSize;

        // Read each present field (including nested)
        if (numFlattenedFields > 10) {  // Main module only
            cout << "Reading fields:" << endl;
        }
        
        for (size_t i = 0; i < numFlattenedFields; ++i) {
            bool present = bitmap[i / 8] & (1 << (i % 8));
            if (present) {
                const auto& [fieldPath, fieldPtr] = flattenedFields[i];
                size_t fieldLen = fieldPtr->getLength();
                
                if (numFlattenedFields > 10) {  // Main module only
                    cout << "  " << i << ": " << fieldPath << " (offset: " << writePos << ", length: " << fieldLen << ")" << endl;
                }
                
                in.read(reinterpret_cast<char*>(row.data() + writePos), fieldLen);

                if (in.gcount() != static_cast<std::streamsize>(fieldLen)) {
                    throw std::runtime_error("Failed to read full field from stream");
                }
                
                writePos += fieldLen;
            }
        }
        
        if (numFlattenedFields > 10) {  // Main module only
            cout << "Final write position: " << writePos << " bytes" << endl;
            cout << "=== END READING ROW DEBUG ===" << endl;
        }

        bytesRemaining -= row.size();
        rows.push_back(std::move(row));
    }

    cout << "=== END READING TABLE ROWS DEBUG ===" << endl;
}

// Helper method to read nested objects
size_t DataModule::readNestedObject(
    istream& in, 
    ObjectField* objectField, 
    vector<uint8_t>& row, 
    size_t writePos) {
    
    const auto& nestedFields = objectField->getNestedFields();
    size_t totalBytesRead = 0;
    
    // For now, assume nested objects have fixed sizes
    // You might need to implement a more sophisticated approach for variable-sized nested objects
    
    for (const auto& nestedField : nestedFields) {
        size_t fieldLen = nestedField->getLength();
        in.read(reinterpret_cast<char*>(row.data() + writePos + totalBytesRead), fieldLen);
        
        if (in.gcount() != static_cast<std::streamsize>(fieldLen)) {
            throw std::runtime_error("Failed to read nested field from stream");
        }
        
        totalBytesRead += fieldLen;
    }
    
    return totalBytesRead;
}


void DataModule::readMetadataRows(istream& in) {
    readTableRows(in, header->getMetadataSize(), metaDataFields, metaDataRows);
}

unique_ptr<DataField> DataModule::parseField(const string& name,
                                                const nlohmann::json& definition) {

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

        return make_unique<EnumField>(name, enumValues, length);
    }


    // Handle integers
    else if (type == "integer") {
        if (!definition.contains("format")) {
            throw runtime_error("Integer field missing 'format': " + name);
        }

        std::string format = definition["format"];

        IntegerFormatInfo integerFormat = IntegerField::parseIntegerFormat(format);

        return make_unique<IntegerField>(name, integerFormat);
    }

    // Handle numbers/floats
    else if (type == "number") {
        if (!definition.contains("format")) {
            throw runtime_error("Number field missing 'format': " + name);
        }

        std::string format = definition["format"];
        
        if (format == "float32" || format == "float64") {
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
            return make_unique<StringField>(name, type, length);
        }
        else {
            return make_unique<VarStringField>(name, &stringBuffer);
        }  
    }

    // Handle objects recursively
    else if (type == "object") {
        if (!definition.contains("properties")) {
            throw runtime_error("Object field missing 'properties': " + name);
        }

        vector<unique_ptr<DataField>> subfields;

        for (const auto& [subname, subdef] : definition["properties"].items()) {
            string fullSubName = subname;
            subfields.emplace_back(parseField(fullSubName, subdef));
        }

        return make_unique<ObjectField>(name, std::move(subfields));
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
        
        // Create a temporary field to calculate item size
        auto tempField = parseField("temp", itemDef);
        
        return make_unique<ArrayField>(name, itemDef, minItems, maxItems);
    }

    // Handle schema references ($ref)
    else if (definition.contains("$ref")) {
        std::string refPath = definition["$ref"];
        nlohmann::json resolvedDef = resolveSchemaReference(refPath, header->getSchemaPath());
        
        // Recursively parse the resolved definition
        return parseField(name, resolvedDef);
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

    // Write String Buffer
    writeStringBuffer(out);

    // Write Metadata
    writeMetaData(out);

    // Write Data
    writeData(out);

    streampos moduleEnd = out.tellp();

    header->setModuleSize(static_cast<uint64_t>(moduleEnd - moduleStart));

    if (header->getModuleSize() != (
        header->getHeaderSize() + 
        header->getStringBufferSize() + 
        header->getMetadataSize() + header->getDataSize())) {

            throw std::runtime_error("Found size mismatch when writing data");
        }

    // Update Header
    header->updateHeader(out);
    out.seekp(moduleEnd);

    // Update XrefTable
    xref.addEntry(
        header->getModuleType(), 
        header->getModuleID(), moduleStart, header->getModuleSize());
}

void DataModule::addTableData(
    const nlohmann::json& data, vector<unique_ptr<DataField>>& fields, vector<vector<uint8_t>>& rows) {
    
    // Build flattened field list including nested fields
    std::vector<std::pair<std::string, DataField*>> flattenedFields;
    
    for (const auto& field : fields) {
        if (auto* objectField = dynamic_cast<ObjectField*>(field.get())) {
            // Add nested fields with dot notation
            const auto& nestedFields = objectField->getNestedFields();
            for (const auto& nestedField : nestedFields) {
                std::string fieldPath = field->getName() + "." + nestedField->getName();
                flattenedFields.push_back({fieldPath, nestedField.get()});
            }
        } else {
            // Regular field
            flattenedFields.push_back({field->getName(), field.get()});
        }
    }
    
    size_t numFlattenedFields = flattenedFields.size();

    cout << "The number of fields is: " << numFlattenedFields << endl;

    size_t bitmapSize = (numFlattenedFields + 7) / 8;
    
    // Build bitmap for all fields (including nested)
    std::vector<uint8_t> bitmap(bitmapSize, 0);
    
    // Only show debug for main module metadata, not frame data
    if (numFlattenedFields > 10) {  // Main image module has 15 fields, frames have 7
        cout << "=== WRITING DEBUG (MAIN MODULE) ===" << endl;
        cout << "Building bitmap for " << numFlattenedFields << " fields:" << endl;
        
        for (size_t i = 0; i < numFlattenedFields; ++i) {
            const auto& [fieldPath, fieldPtr] = flattenedFields[i];
            
            // Check if field exists in data (handle nested paths)
            bool present = fieldExistsInData(data, fieldPath);
            
            cout << "  " << i << ": " << fieldPath << " (type: " << fieldPtr->getType() << ") - present: " << (present ? "YES" : "NO") << endl;
            
            if (present) {
                bitmap[i / 8] |= (1 << (i % 8));
            }
        }
        
        cout << "Bitmap: ";
        for (size_t i = 0; i < bitmapSize; ++i) {
            cout << std::hex << static_cast<int>(bitmap[i]) << " ";
        }
        cout << std::dec << endl;
    } else {
        // For frames, just build bitmap silently
        for (size_t i = 0; i < numFlattenedFields; ++i) {
            const auto& [fieldPath, fieldPtr] = flattenedFields[i];
            bool present = fieldExistsInData(data, fieldPath);
            if (present) {
                bitmap[i / 8] |= (1 << (i % 8));
            }
        }
    }
    
    // Calculate row size including nested objects
    size_t maxRowSize = 0;
    for (const auto& [fieldPath, fieldPtr] : flattenedFields) {
        if (fieldExistsInData(data, fieldPath)) {
            maxRowSize += fieldPtr->getLength();
        }
    }
    
    if (numFlattenedFields > 10) {  // Main module only
        cout << "Row size: bitmap(" << bitmapSize << ") + data(" << maxRowSize << ") = " << (bitmapSize + maxRowSize) << " bytes" << endl;
    }
    
    vector<uint8_t> row(bitmapSize + maxRowSize, 0);
    
    // Write in the bitmap at the start of the row
    memcpy(row.data(), bitmap.data(), bitmapSize);
    
    size_t offset = bitmapSize;
    
    if (numFlattenedFields > 10) {  // Main module only
        cout << "Encoding fields:" << endl;
    }
    
    // Encode all fields (including nested)
    for (const auto& [fieldPath, fieldPtr] : flattenedFields) {
        if (fieldExistsInData(data, fieldPath)) {
            nlohmann::json value = getNestedValue(data, fieldPath);
            if (numFlattenedFields > 10) {  // Main module only
                cout << "  " << fieldPath << " -> " << value.dump() << " (offset: " << offset << ", length: " << fieldPtr->getLength() << ")" << endl;
            }
            fieldPtr->encodeToBuffer(value, row, offset);
            offset += fieldPtr->getLength();
        }
    }
    
    if (numFlattenedFields > 10) {  // Main module only
        cout << "Final row size: " << offset << " bytes" << endl;
        cout << "=== END WRITING DEBUG ===" << endl;
    }
    
    row.resize(offset);
    rows.push_back(std::move(row));
}

bool DataModule::fieldExistsInData(const nlohmann::json& data, const std::string& fieldPath) {
    // Handle "image_structure.dimensions" -> data["image_structure"]["dimensions"]
    size_t dotPos = fieldPath.find('.');
    if (dotPos == std::string::npos) {
        return data.contains(fieldPath) && !data[fieldPath].is_null();
    }
    
    std::string parentField = fieldPath.substr(0, dotPos);
    std::string childField = fieldPath.substr(dotPos + 1);
    
    if (!data.contains(parentField) || !data[parentField].is_object()) {
        return false;
    }
    
    return data[parentField].contains(childField) && !data[parentField][childField].is_null();
}

nlohmann::json DataModule::getNestedValue(const nlohmann::json& data, const std::string& fieldPath) {
    size_t dotPos = fieldPath.find('.');
    if (dotPos == std::string::npos) {
        return data[fieldPath];
    }
    
    std::string parentField = fieldPath.substr(0, dotPos);
    std::string childField = fieldPath.substr(dotPos + 1);
    
    return data[parentField][childField];
}


void DataModule::addMetaData(const nlohmann::json& data) {
    addTableData(data, metaDataFields, metaDataRows);
}

void DataModule::printTableData(
    ostream& out, const vector<unique_ptr<DataField>>& fields, const vector<vector<uint8_t>>& rows) const{
    
    // Build flattened field list including nested fields (same logic as readTableRows)
    std::vector<std::pair<std::string, DataField*>> flattenedFields;
    
    for (const auto& field : fields) {
        if (auto* objectField = dynamic_cast<ObjectField*>(field.get())) {
            // Add nested fields with dot notation
            const auto& nestedFields = objectField->getNestedFields();
            for (const auto& nestedField : nestedFields) {
                std::string fieldPath = field->getName() + "." + nestedField->getName();
                flattenedFields.push_back({fieldPath, nestedField.get()});
            }
        } else {
            // Regular field
            flattenedFields.push_back({field->getName(), field.get()});
        }
    }
    
    size_t numFlattenedFields = flattenedFields.size();
    size_t bitmapSize = (numFlattenedFields + 7) / 8;

    for (const auto& row : rows) {
        // Read bitmap
        std::vector<uint8_t> bitmap(bitmapSize);
        std::memcpy(bitmap.data(), row.data(), bitmapSize);

        // Start reading after bitmap
        size_t offset = bitmapSize;
        nlohmann::json rowJson = nlohmann::json::object();

        // For each flattened field, check bitmap before decoding
        for (size_t i = 0; i < numFlattenedFields; ++i) {
            bool present = bitmap[i / 8] & (1 << (i % 8));

            if (present) {
                const auto& [fieldPath, fieldPtr] = flattenedFields[i];
                rowJson[fieldPath] = fieldPtr->decodeFromBuffer(row, offset);
                offset += fieldPtr->getLength();
            } else {
                const auto& [fieldPath, fieldPtr] = flattenedFields[i];
                rowJson[fieldPath] = nullptr;
            }
        }

        out << rowJson.dump(2) << "\n";
    }

    cout << "=== END PRINTING TABLE DATA DEBUG ===" << endl;
}

void DataModule::printMetadata(std::ostream& out) const {
    printTableData(out, metaDataFields, metaDataRows);
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
    cout << "=== getMetadataAsJson() DEBUG ===" << endl;
    
    nlohmann::json metadataArray = nlohmann::json::array();

    size_t numFields = metaDataFields.size();
    size_t bitmapSize = (numFields + 7) / 8;
    
    cout << "Using OLD field structure:" << endl;
    cout << "  numFields: " << numFields << endl;
    cout << "  bitmapSize: " << bitmapSize << " bytes" << endl;
    cout << "  metaDataFields names:" << endl;
    for (size_t i = 0; i < numFields; ++i) {
        cout << "    " << i << ": " << metaDataFields[i]->getName() << " (type: " << metaDataFields[i]->getType() << ", length: " << metaDataFields[i]->getLength() << ")" << endl;
    }
    
    for (const auto& row : metaDataRows) {
        cout << "Processing row of size: " << row.size() << " bytes" << endl;
        size_t offset = 0;

        // Read bitmap from start of row
        std::vector<uint8_t> bitmap(bitmapSize);
        memcpy(bitmap.data(), row.data(), bitmapSize);
        offset += bitmapSize;
        
        cout << "  Read bitmap (" << bitmapSize << " bytes): ";
        for (size_t i = 0; i < bitmapSize; ++i) {
            cout << std::hex << static_cast<int>(bitmap[i]) << " ";
        }
        cout << std::dec << endl;
        cout << "  Starting offset after bitmap: " << offset << endl;

        nlohmann::json rowJson = nlohmann::json::object();

        cout << "  Decoding fields:" << endl;
        for (size_t i = 0; i < numFields; ++i) {
            bool present = bitmap[i / 8] & (1 << (i % 8));
            cout << "    " << i << ": " << metaDataFields[i]->getName() << " - present: " << (present ? "YES" : "NO") << " (offset: " << offset << ")" << endl;
            
            if (present) {
                cout << "      Calling decodeFromBuffer with offset: " << offset << endl;
                rowJson[metaDataFields[i]->getName()] =
                    metaDataFields[i]->decodeFromBuffer(row, offset);
                offset += metaDataFields[i]->getLength();
                cout << "      New offset after field: " << offset << endl;
            } else {
                rowJson[metaDataFields[i]->getName()] = nullptr; // or skip entirely
            }
        }

        metadataArray.push_back(rowJson);
    }
    
    cout << "=== END getMetadataAsJson() DEBUG ===" << endl;
    return metadataArray;
}

// Initialize static member
std::unordered_map<std::string, nlohmann::json> DataModule::schemaCache;

nlohmann::json DataModule::resolveSchemaReference(const std::string& refPath, const std::string& baseSchemaPath) {
    // Check if schema is already cached
    if (schemaCache.find(refPath) != schemaCache.end()) {
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
    
    // Load the referenced schema
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open referenced schema file: " + fullPath);
    }
    
    nlohmann::json referencedSchema;
    file >> referencedSchema;
    
    // Cache the loaded schema
    schemaCache[refPath] = referencedSchema;
    
    return referencedSchema;
}