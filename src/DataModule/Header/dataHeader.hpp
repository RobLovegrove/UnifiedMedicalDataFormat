#ifndef DATAHEADER_HPP
#define DATAHEADER_HPP

#include <string>
#include <fstream>
#include <expected>

#include "../../Utility/uuid.hpp"
#include "../../Utility/moduleType.hpp"
#include "../../Utility/Compression/CompressionType.hpp"
#include "../../Utility/Encryption/EncryptionManager.hpp"

enum class HeaderFieldType : uint8_t {
    HeaderSize = 1,
    StringSize = 2,
    MetadataSize = 3,
    DataSize = 4,
    IsCurrent = 5,
    PreviousVersion = 6,
    ModuleType = 7,
    SchemaPath = 8,
    MetadataCompression = 9,
    DataCompression = 10,
    EncryptionType = 11,
    BaseSalt = 12,
    ModuleSalt = 13,
    MemoryCost = 14,
    TimeCost = 15,
    Parallelism = 16,
    IV = 17,
    AuthTag = 18,
    Endianness = 19,
    ModuleID = 20
};

struct DataHeader {
protected:

    uint32_t headerSize = 0;
    uint64_t metaDataSize = 0;
    uint64_t dataSize = 0;
    uint64_t stringBufferSize = 0;

    uint64_t moduleStartOffset;
    uint64_t totalModuleSize = 0;

    bool isCurrent = true; // 1 = current, 0 = previous/old version
    uint64_t previousVersion = 0;
    
    std::streampos headerSizePos = 0;
    std::streampos metadataSizePos = 0;
    std::streampos dataSizePos = 0;
    std::streampos stringBufferSizePos = 0;

    std::streampos authTagPos = 0;

    std::streampos dataOffsetPos = 0;
    std::streampos stringOffsetPos = 0;

    ModuleType moduleType;
    std::string schemaPath;
    CompressionType metadataCompression;
    CompressionType dataCompression;
    EncryptionData encryptionData;
    bool littleEndian;
    UUID moduleID;


    void writeTLVString(std::ostream& out, HeaderFieldType type, const std::string& value) const;
    void writeTLVBool(std::ostream& out, HeaderFieldType type, bool value) const;
    std::streampos writeTLVFixed(std::ostream& out, HeaderFieldType type, const void* data, uint32_t size) const;
    
    std::expected<std::streampos, std::string> findTLVOffset(std::fstream& fileStream, HeaderFieldType type);
    // virtual bool handleExtraField(HeaderFieldType, const std::vector<char>&) = 0;

public:

// GETTTERS AND SETTERS

    ModuleType getModuleType() const { return moduleType; }
    void setModuleType(ModuleType type) { moduleType = type; }

    std::string getSchemaPath() const { return schemaPath; }
    void setSchemaPath(std::string path) { schemaPath = path; }

    UUID getModuleID() const { return moduleID; }
    void setModuleID(UUID id) { moduleID = id; }

    uint64_t getModuleStartOffset() const { return moduleStartOffset; }
    void setModuleStartOffset(uint64_t offset) { moduleStartOffset = offset; }

    void setModuleSize(uint64_t size) { totalModuleSize = size; }
    uint64_t getModuleSize() const;

    uint32_t getHeaderSize() const { return headerSize; }
    void setHeaderSize(uint32_t size) { headerSize = size; }

    uint64_t getMetadataSize() const { return metaDataSize; }
    void setMetadataSize(uint64_t size) { metaDataSize = size; }

    uint64_t getDataSize() const { return dataSize; }
    void setDataSize(uint64_t size) { dataSize = size; }

    uint64_t getStringBufferSize() const { return stringBufferSize; }
    void setStringBufferSize(uint64_t size) { stringBufferSize = size; }

    CompressionType getMetadataCompression() const { return metadataCompression; }
    void setMetadataCompression(CompressionType compression) { metadataCompression = compression; }

    CompressionType getDataCompression() const { return dataCompression; }
    void setDataCompression(CompressionType compression) { dataCompression = compression; }

    bool getLittleEndian() const { return littleEndian; }
    void setLittleEndian(bool lE) { littleEndian = lE; }

    virtual uint64_t getAdditionalOffset() const { return 0; }
    virtual void seAdditionalOffset(uint64_t) {}

    uint64_t getPrevious() const { return previousVersion; }
    void setPrevious(uint64_t offset) { previousVersion = offset; }

    EncryptionData getEncryptionData() const { return encryptionData; }
    void setEncryptionData(EncryptionData data) { encryptionData = data; }

// METHODS
    virtual ~DataHeader() = default;

    void writeToFile(std::ostream& out);

    virtual void updateHeader(std::ostream& out);

    void readHeaderSize(std::istream& in);
    void readDataHeader(std::istream& in);

    bool updateIsCurrent(bool newIsCurrent, std::fstream& fileStream);

    friend std::ostream& operator<<(std::ostream& os, const DataHeader& header);

};


#endif





/*

HeaderSize – HeaderFieldType::HeaderSize
uint8_t tag
uint32_t length
uint32_t value (placeholder at first, overwritten later)

DataSize – HeaderFieldType::DataSize
uint8_t tag
uint32_t length
uint64_t value

ModuleType – HeaderFieldType::ModuleType
uint8_t tag
uint32_t length
std::string value (variable size)

SchemaPath – HeaderFieldType::SchemaPath
uint8_t tag
uint32_t length
std::string value (variable size)

Compression – HeaderFieldType::Compression
uint8_t tag
uint32_t length
bool (stored as uint8_t)

Endianness – HeaderFieldType::Endianness
uint8_t tag
uint32_t length
bool (stored as uint8_t)

ModuleID – HeaderFieldType::ModuleID
uint8_t tag
uint32_t length
16-byte UUID (std::array<uint8_t, 16>)



*/