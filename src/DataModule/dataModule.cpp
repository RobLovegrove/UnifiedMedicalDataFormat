#include "dataModule.hpp"
#include "./Image/imageData.hpp"
#include "./Tabular/tabularData.hpp"
#include "../Utility/uuid.hpp"
#include "../Xref/xref.hpp"
#include "stringBuffer.hpp"
#include "SchemaResolver.hpp"
#include "../Utility/Compression/ZstdCompressor.hpp"
#include "../Utility/Encryption/encryptionManager.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

using namespace std;

DataModule::DataModule(const string& schemaPath, UUID uuid, ModuleType type, EncryptionData encryptionData) {

    header = make_unique<DataHeader>();
    header->setModuleType(type);
    header->setSchemaPath(schemaPath);
    header->setModuleID(uuid);
    header->setMetadataCompression(CompressionType::ZSTD);
    header->setEncryptionData(encryptionData);

    ifstream file = openSchemaFile(schemaPath);
    file >> schemaJson;
    
}

DataModule::DataModule(
    const string& schemaPath, const nlohmann::json& schemaJson, UUID uuid, ModuleType type, EncryptionData encryptionData) 
    : schemaJson(schemaJson) {

    header = make_unique<DataHeader>();
    header->setModuleType(type);
    header->setSchemaPath(schemaPath);
    header->setModuleID(uuid);
    header->setMetadataCompression(CompressionType::ZSTD);
    header->setEncryptionData(encryptionData);
}

void DataModule::initialise() {
    try {
        parseSchema(schemaJson);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse schema: " + std::string(e.what()));
    }
}

const nlohmann::json& DataModule::getSchema() const {
    return schemaJson;
}

void DataModule::parseSchema(const nlohmann::json& schemaJson) {

    parseSchemaHeader(schemaJson);

    if (schemaJson.contains("required")) {
        const auto& required = schemaJson["required"];
        for (const auto& field : required) {
            if (!schemaJson["properties"].contains(field)) {
                throw std::runtime_error("Schema must contain 'required' field: " + field.dump());
            }
        }
    }

    const auto& props = schemaJson["properties"];

    // Parse metadata
    if (props.contains("metadata")) {

        if (props.at("metadata").contains("required")) {
            metadataRequired = props.at("metadata").at("required");
        }

        for (const auto& field : metadataRequired) {
            if (!props.at("metadata").at("properties").contains(field)) {
                throw std::runtime_error("Schema must contain 'required' field: " + field);
            }
        }

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
    istream& in, uint64_t moduleStartOffset, uint8_t moduleType, EncryptionData encryptionData) {

    unique_ptr<DataHeader> dmHeader = make_unique<DataHeader>();

    if (moduleType == static_cast<uint8_t>(ModuleType::Frame)) {
        EncryptionData frameEncryptionData;
        frameEncryptionData.encryptionType = EncryptionType::NONE;
        dmHeader->setEncryptionData(frameEncryptionData);
    }
    else {
        dmHeader->setEncryptionData(encryptionData);
    }
    
    dmHeader->readDataHeader(in);



    unique_ptr<DataModule> dm;

    try {
        switch (static_cast<ModuleType>(moduleType)) {
        case ModuleType::Tabular:
            dm = make_unique<TabularData>(dmHeader->getSchemaPath(), dmHeader->getModuleID(), dmHeader->getEncryptionData());
            break;
        case ModuleType::Image:
            dm = make_unique<ImageData>(dmHeader->getSchemaPath(), dmHeader->getModuleID(), dmHeader->getEncryptionData());
            break;
        case ModuleType::Frame:
            dm = make_unique<FrameData>(dmHeader->getSchemaPath(), dmHeader->getModuleID(), dmHeader->getEncryptionData());
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

    if (dm->header->getEncryptionData().encryptionType != EncryptionType::NONE) {


        // Decrypt the data
        std::istringstream decryptedStream = dm->decryptData(in);

        dm->readDecryptedMetadataAndData(decryptedStream);
        
    }
    else {
        dm->readDecryptedMetadataAndData(in);
    }

    return dm;
}

void DataModule::readDecryptedMetadataAndData(istream& in) {
    if (header->getMetadataCompression() == CompressionType::ZSTD) {
        readCompressedMetadata(in);
    }
    else {
        readStringBufferAndMetadata(in);
    }



    if (header->getDataSize() > 0) {

        
        // Read in data
        std::vector<uint8_t> buffer(header->getDataSize());
    
        in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        


        // Handle compression if needed
        if (header->getDataCompression() == CompressionType::ZSTD) {
            std::vector<uint8_t> decompressedData = ZstdCompressor::decompress(buffer);

            // Update the data size with the decompressed data size
            header->setDataSize(decompressedData.size());

            std::istringstream inputStream {
                std::string(reinterpret_cast<char*>(decompressedData.data()), decompressedData.size())
            };

            readData(inputStream);
        }
        else {
            std::istringstream inputStream(
                std::string(reinterpret_cast<char*>(buffer.data()), buffer.size())
            );
            
            readData(inputStream);
        }
    }
}


std::istringstream DataModule::decryptData(istream& in) {
    // Read in the encrypted data
    std::vector<uint8_t> encryptedData(header->getDataSize());

    in.read(reinterpret_cast<char*>(encryptedData.data()), encryptedData.size());



    if (in.gcount() != static_cast<std::streamsize>(encryptedData.size())) {
        throw std::runtime_error("Failed to read full data block");
    }

    EncryptionData encryptionData = header->getEncryptionData();

    std::vector<uint8_t> combinedSalt;
    combinedSalt.reserve(encryptionData.baseSalt.size() + encryptionData.moduleSalt.size());
    combinedSalt.insert(combinedSalt.end(), encryptionData.baseSalt.begin(), encryptionData.baseSalt.end());
    combinedSalt.insert(combinedSalt.end(), encryptionData.moduleSalt.begin(), encryptionData.moduleSalt.end());
    
    // Derive the encryption key using the correct crypto_pwhash function
    auto derivedKey = EncryptionManager::deriveKeyArgon2id(
        encryptionData.masterPassword, 
        combinedSalt,
        encryptionData.memoryCost, encryptionData.timeCost, encryptionData.parallelism
    );

    // Decrypt the data
    std::vector<uint8_t> decryptedData = EncryptionManager::decryptAES256GCM(
        encryptedData,           
        derivedKey,              
        encryptionData.iv,       
        encryptionData.authTag
    );

    if (decryptedData.empty()) {
        throw std::runtime_error("Failed to decrypt data");
    }
    
    // Convert the decrypted data to an istream
    std::istringstream decryptedStream(
        std::string(reinterpret_cast<const char*>(decryptedData.data()), 
                    decryptedData.size())
    );

    // Read in the string buffer and metadata and data sizes
    uint64_t stringBufferSize, metadataSize, dataSize;
    decryptedStream.read(reinterpret_cast<char*>(&stringBufferSize), sizeof(uint64_t));
    decryptedStream.read(reinterpret_cast<char*>(&metadataSize), sizeof(uint64_t));
    decryptedStream.read(reinterpret_cast<char*>(&dataSize), sizeof(uint64_t));

    // Set header sizes
    header->setStringBufferSize(stringBufferSize);
    header->setMetadataSize(metadataSize);
    header->setDataSize(dataSize);



    return decryptedStream;
}


void DataModule::readCompressedMetadata(istream& in) {
    std::vector<uint8_t> buffer(header->getMetadataSize());
    in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    
    if (in.gcount() != static_cast<std::streamsize>(buffer.size())) {
        throw std::runtime_error("Failed to read full metadata block");
    }

    // Decompress the metadata
    std::vector<uint8_t> compressedData = ZstdCompressor::decompress(buffer);

    std::istringstream inputStream {
        std::string(reinterpret_cast<char*>(compressedData.data()), compressedData.size())
    };

    // Read the string buffer and metadata sizes
    uint64_t stringBufferSize;
    uint64_t metadataSize;

    inputStream.read(reinterpret_cast<char*>(&stringBufferSize), sizeof(stringBufferSize));
    inputStream.read(reinterpret_cast<char*>(&metadataSize), sizeof(metadataSize));

    // Set header sizes
    header->setStringBufferSize(stringBufferSize);
    header->setMetadataSize(metadataSize);

    readStringBufferAndMetadata(inputStream);
}

void DataModule::readStringBufferAndMetadata(istream& in) {
    // Only read string buffer if there's actually content
    if (header->getStringBufferSize() > 0) {
        stringBuffer.readFromFile(in, header->getStringBufferSize());
    }

    // Read in metadata
    if (header->getMetadataSize() > 0) {
        std::vector<uint8_t> buffer(header->getMetadataSize());
        in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

        if (in.gcount() != static_cast<std::streamsize>(buffer.size())) {
            throw std::runtime_error("Failed to read full metadata block");
        }

        // Handle compression if needed
        if (header->getMetadataCompression() != CompressionType::RAW) {
        }

        std::istringstream inputStream(
            std::string(reinterpret_cast<char*>(buffer.data()), buffer.size())
        );

        readMetadataRows(inputStream);
    }

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


    


    size_t bytesRemaining = dataSize;

    while (bytesRemaining > 0) {
        // Read bitmap for all fields (including nested)
        std::vector<uint8_t> bitmap(bitmapSize);
        in.read(reinterpret_cast<char*>(bitmap.data()), bitmapSize);
        if (in.gcount() != static_cast<std::streamsize>(bitmapSize)) {
            throw std::runtime_error("Truncated bitmap");
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
        

        
        // Start a row with the bitmap
        std::vector<uint8_t> row(rowSize);

        // Start writing at position 0
        size_t writePos = 0;
        memcpy(row.data(), bitmap.data(), bitmapSize);
        writePos += bitmapSize;

        // Read each present field (including nested)
        for (size_t i = 0; i < numFlattenedFields; ++i) {
            bool present = bitmap[i / 8] & (1 << (i % 8));
            if (present) {
                const auto& [fieldPath, fieldPtr] = flattenedFields[i];
                size_t fieldLen = fieldPtr->getLength();
                
                in.read(reinterpret_cast<char*>(row.data() + writePos), fieldLen);

                if (in.gcount() != static_cast<std::streamsize>(fieldLen)) {
                    throw std::runtime_error("Failed to read full field from stream");
                }
                
                writePos += fieldLen;
            }
        }

        bytesRemaining -= row.size();
        rows.push_back(std::move(row));
    }

    
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

    // Handle schema references ($ref) first - before type checking
    if (definition.contains("$ref")) {
        std::string refPath = definition["$ref"];
        
        // Keep the resolved path on the resolver's stack while we parse the referenced schema
        std::string fullPath = SchemaResolver::beginReference(refPath, header->getSchemaPath());
        nlohmann::json resolvedDef = SchemaResolver::getSchemaByResolvedPath(fullPath);
        auto field = parseField(name, resolvedDef);
        SchemaResolver::endReference();
        return field;
    }

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

        std::optional<int64_t> minValue = std::nullopt;
        std::optional<int64_t> maxValue = std::nullopt;

        if (definition.contains("minimum")) {
            minValue = definition["minimum"];
        }

        if (definition.contains("maximum")) {
            maxValue = definition["maximum"];
        }

        return make_unique<IntegerField>(name, integerFormat, minValue, maxValue);
    }

    // Handle numbers/floats
    else if (type == "number") {
        if (!definition.contains("format")) {
            throw runtime_error("Number field missing 'format': " + name);
        }

        std::string format = definition["format"];

        std::optional<int64_t> minValue = std::nullopt; 
        std::optional<int64_t> maxValue = std::nullopt;

        if (format == "float32" || format == "float64") {
            if (definition.contains("minimum")) {
                minValue = definition["minimum"];
            }
            if (definition.contains("maximum")) {
                maxValue = definition["maximum"];
            }
            return make_unique<FloatField>(name, format, minValue, maxValue);
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

        std::vector<std::string> requiredFields;

        if (!definition.contains("properties")) {
            throw runtime_error("Object field missing 'properties': " + name);
        }

        if (definition.contains("required")) {
            requiredFields = definition["required"];
            for (const auto& field : requiredFields) {
                if (!definition["properties"].contains(field)) {
                    throw std::runtime_error("ObjectField '" + name + "' missing required field: " + field);
                }
            }
        }

        vector<unique_ptr<DataField>> subfields;

        for (const auto& [subname, subdef] : definition["properties"].items()) {
            string fullSubName = subname;
            subfields.emplace_back(parseField(fullSubName, subdef));
        }

        return make_unique<ObjectField>(name, std::move(subfields), std::move(requiredFields));
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

    // Handle bools
    // else if (type == "bool") {
    //     size_t length = 1;
    //     return make_unique<IntegerField>(name, IntegerField::parseIntegerFormat("uint8"), std::nullopt, std::nullopt);
    // }

    else {
        throw runtime_error("Unsupported field type: " + type);
    }
}

void DataModule::writeBinary(std:: streampos absoluteModuleStart, std::ostream& out, XRefTable& xref) {
    
    this->absoluteModuleStart = absoluteModuleStart;

    streampos moduleStart = out.tellp();
    header->setModuleStartOffset(static_cast<uint64_t>(moduleStart));

    // Write header
    header->writeToFile(out);

    if (header->getModuleType() != ModuleType::Frame && header->getEncryptionData().encryptionType != EncryptionType::NONE) {
        // Encrypted
        if (header->getMetadataCompression() == CompressionType::ZSTD) {

            std::stringstream metadataBuffer;
            writeCompressedMetadata(metadataBuffer);

            std::stringstream dataBuffer;
            writeData(dataBuffer); // Compression handled in writeData

            encryptModule(metadataBuffer, dataBuffer, out);
            
        }
        else {

            // Encrypted but metadata not compressed
            std::stringstream metadataBuffer;
            writeStringBuffer(metadataBuffer);
            writeMetaData(metadataBuffer);

            std::stringstream dataBuffer;
            writeData(dataBuffer);



            encryptModule(metadataBuffer, dataBuffer, out);
        }
    }
    else {
        // Not encrypted
        if (header->getMetadataCompression() == CompressionType::ZSTD) {
            // Compressed but not encrypted
            writeCompressedMetadata(out);
        }
        else {
            // Not compressed or encrypted
            // Write String Buffer
            writeStringBuffer(out);
            // Write Metadata
            writeMetaData(out);
        }
        writeData(out);
    }

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
        header->getModuleID(), absoluteModuleStart, header->getModuleSize());

}

void DataModule::writeCompressedMetadata(std::ostream& metadataStream) {

    uint64_t stringBufferSize = stringBuffer.getSize();
    uint64_t metadataSize = 0;
    for (const auto& row : metaDataRows) {
        metadataSize += row.size();
    }

    // Create a buffer and write metadata and stringBuffer sizes
    std::stringstream buffer;
    
    buffer.write(reinterpret_cast<const char*>(&stringBufferSize), sizeof(stringBufferSize));
    buffer.write(reinterpret_cast<const char*>(&metadataSize), sizeof(metadataSize));
    
    // Write string buffer and metadata to buffer
    writeStringBuffer(buffer);
    writeMetaData(buffer);

    // Convert to bytes
    std::vector<uint8_t> dataBytes {
        std::istreambuf_iterator<char>(buffer),
        std::istreambuf_iterator<char>()
    };

    // Compress the buffer
    std::vector<uint8_t> compressedData = ZstdCompressor::compress(dataBytes);

    size_t compressedDataSize = compressedData.size();

    // Write the compressed data to the output stream
    metadataStream.write(reinterpret_cast<const char*>(compressedData.data()), compressedDataSize);

    // Update header with compressed data size
    header->setStringBufferSize(0);
    header->setMetadataSize(compressedDataSize);
}

void DataModule::encryptModule(std::stringstream& metadataStream,
    std::stringstream& dataStream,
    std::ostream& out) 
{
    std::vector<uint8_t> encryptionBuffer;

    uint64_t stringBufferSize = header->getStringBufferSize();
    uint64_t metadataSize     = header->getMetadataSize();
    uint64_t dataSize         = header->getDataSize();

    // Pre-allocate
    encryptionBuffer.reserve(sizeof(uint64_t) * 3 + stringBufferSize + metadataSize + dataSize);

    auto appendUint64 = [&](uint64_t value) {
    uint8_t buf[sizeof(uint64_t)];
    std::memcpy(buf, &value, sizeof(value));
    encryptionBuffer.insert(encryptionBuffer.end(), buf, buf + sizeof(value));
    };

    // Append sizes
    appendUint64(stringBufferSize);
    appendUint64(metadataSize);
    appendUint64(dataSize);

    // Append actual metadata and data streams
    std::string metaStr = metadataStream.str();
    std::string dataStr = dataStream.str();
    encryptionBuffer.insert(encryptionBuffer.end(), metaStr.begin(), metaStr.end());
    encryptionBuffer.insert(encryptionBuffer.end(), dataStr.begin(), dataStr.end());

    // Encryption parameters
    EncryptionData encryptionData = header->getEncryptionData();

    std::vector<uint8_t> combinedSalt;
    combinedSalt.reserve(encryptionData.baseSalt.size() + encryptionData.moduleSalt.size());
    combinedSalt.insert(combinedSalt.end(), encryptionData.baseSalt.begin(), encryptionData.baseSalt.end());
    combinedSalt.insert(combinedSalt.end(), encryptionData.moduleSalt.begin(), encryptionData.moduleSalt.end());

    auto derivedKey = EncryptionManager::deriveKeyArgon2id(
    encryptionData.masterPassword, 
    combinedSalt,
    encryptionData.memoryCost,
    encryptionData.timeCost,
    encryptionData.parallelism
    );

    // Encrypt the entire buffer
    std::vector<uint8_t> encryptedData = EncryptionManager::encryptAES256GCM(
    encryptionBuffer,
    derivedKey,
    encryptionData.iv,
    encryptionData.authTag
    );

    // Update encryption data back into header
    header->setEncryptionData(encryptionData);

    // Write encrypted data to output
    out.write(reinterpret_cast<const char*>(encryptedData.data()), encryptedData.size());

    // Update header sizes (only encrypted payload is stored now)
    header->setStringBufferSize(0);
    header->setMetadataSize(0);
    header->setDataSize(encryptedData.size());
}


void DataModule::addTableData(
    const nlohmann::json& data, vector<unique_ptr<DataField>>& fields, 
    vector<vector<uint8_t>>& rows, vector<std::string>& requiredFields) {

    for (const auto& field : requiredFields) {
        if (!data.contains(field)) {
            throw std::runtime_error("Data missing required field: " + field);
        }
    }
    
    // Validate object fields' required subfields before flattening
    for (const auto& fieldPtrTop : fields) {
        if (auto* objectField = dynamic_cast<ObjectField*>(fieldPtrTop.get())) {
            const std::string& objName = objectField->getName();
            if (!data.contains(objName) || !data.at(objName).is_object()) {
                throw std::runtime_error("Invalid value for field: " + objName);
            }
            if (!objectField->validateValue(data.at(objName))) {
                throw std::runtime_error("Invalid value for field: " + objName);
            }
        }
    }
    
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
    
    // Build bitmap for all fields (including nested)
    std::vector<uint8_t> bitmap(bitmapSize, 0);
    
    for (size_t i = 0; i < numFlattenedFields; ++i) {
        const auto& [fieldPath, fieldPtr] = flattenedFields[i];
        bool present = fieldExistsInData(data, fieldPath);
        if (present) {
            bitmap[i / 8] |= (1 << (i % 8));
        }
    }
    
    // Calculate row size including nested objects
    size_t maxRowSize = 0;
    for (const auto& [fieldPath, fieldPtr] : flattenedFields) {
        if (fieldExistsInData(data, fieldPath)) {
            maxRowSize += fieldPtr->getLength();
        }
    }
    
    vector<uint8_t> row(bitmapSize + maxRowSize, 0);
    
    // Write in the bitmap at the start of the row
    memcpy(row.data(), bitmap.data(), bitmapSize);
    
    size_t offset = bitmapSize;
    
    // Encode all fields (including nested)
    for (const auto& [fieldPath, fieldPtr] : flattenedFields) {
        if (fieldExistsInData(data, fieldPath)) {
            nlohmann::json value = getNestedValue(data, fieldPath);
            if (!fieldPtr->validateValue(value)) {
                throw std::runtime_error("Invalid value for field: " + fieldPath);
            }
            fieldPtr->encodeToBuffer(value, row, offset);
            offset += fieldPtr->getLength();
        }
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

    if (data.is_array()) {
        // Handle array of metadata rows
        for (const auto& row : data) {
            addTableData(row, metaDataFields, metaDataRows, metadataRequired);
        }
    } else {
        // Handle single metadata row (backward compatibility)
        addTableData(data, metaDataFields, metaDataRows, metadataRequired);
    }
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


}

void DataModule::printMetadata(std::ostream& out) const {
    printTableData(out, metaDataFields, metaDataRows);
}

// Template method that handles common functionality
ModuleData DataModule::getModuleData() const {
    return {
        getMetadataAsJson(),
        getModuleSpecificData()
    };
}

nlohmann::json DataModule::getTableDataAsJson(
    const vector<std::string>& requiredFields,
    const vector<vector<uint8_t>>& rows, 
    const vector<unique_ptr<DataField>>& fields) const {

    nlohmann::json dataArray = nlohmann::json::array();

    // Build flattened field list including nested fields (same logic as readTableRows)
    std::vector<std::pair<std::string, DataField*>> flattenedFields;
    
    // Also keep track of the original field structure for reconstruction
    std::vector<std::pair<std::string, std::string>> fieldMapping; // parentField, childField
    
    for (const auto& field : fields) {
        if (auto* objectField = dynamic_cast<ObjectField*>(field.get())) {
            // Add nested fields with dot notation for bitmap calculation
            const auto& nestedFields = objectField->getNestedFields();
            for (const auto& nestedField : nestedFields) {
                std::string fieldPath = field->getName() + "." + nestedField->getName();
                flattenedFields.push_back({fieldPath, nestedField.get()});
                fieldMapping.push_back({field->getName(), nestedField->getName()});
            }
        } else {
            // Regular field
            flattenedFields.push_back({field->getName(), field.get()});
            fieldMapping.push_back({field->getName(), ""}); // Empty string means no parent
        }
    }
    
    size_t numFlattenedFields = flattenedFields.size();
    size_t bitmapSize = (numFlattenedFields + 7) / 8;
    
    for (const auto& row : rows) {
        size_t offset = 0;

        // Read bitmap from start of row
        std::vector<uint8_t> bitmap(bitmapSize);
        memcpy(bitmap.data(), row.data(), bitmapSize);
        offset += bitmapSize;

        nlohmann::json rowJson = nlohmann::json::object();

        for (size_t i = 0; i < numFlattenedFields; ++i) {
            bool present = bitmap[i / 8] & (1 << (i % 8));
            if (present) {
                const auto& [fieldPath, fieldPtr] = flattenedFields[i];
                const auto& [parentField, childField] = fieldMapping[i];
                
                if (childField.empty()) {
                    // Regular field - add directly to row
                    rowJson[parentField] = fieldPtr->decodeFromBuffer(row, offset);
                } else {
                    // Nested field - create parent object if it doesn't exist
                    if (!rowJson.contains(parentField)) {
                        rowJson[parentField] = nlohmann::json::object();
                    }
                    rowJson[parentField][childField] = fieldPtr->decodeFromBuffer(row, offset);
                }
                offset += fieldPtr->getLength();
            } else {
                // Handle missing fields - could set to null or skip entirely
                // For now, we'll skip missing fields to keep the output clean
            }
        }

        dataArray.push_back(rowJson);
    }
    for (const auto& row : dataArray) {
        for (const auto& field : requiredFields) {
            if (!row.contains(field)) {
                throw std::runtime_error("Data missing required field: " + field);
            }
        }
    }
    
    return dataArray;
}


// Helper method to reconstruct metadata from stored fields
nlohmann::json DataModule::getMetadataAsJson() const {
    nlohmann::json metadataArray = nlohmann::json::array();

    return getTableDataAsJson(metadataRequired, metaDataRows, metaDataFields);
}

// Note: Schema caching is now handled by SchemaResolver class

// nlohmann::json DataModule::resolveSchemaReference(const std::string& refPath, const std::string& baseSchemaPath) {
//     // Delegate to SchemaResolver for circular reference detection and caching
//     return SchemaResolver::resolveReference(refPath, baseSchemaPath);
// }