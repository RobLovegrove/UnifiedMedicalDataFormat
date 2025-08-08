#ifndef DATAHEADER_HPP
#define DATAHEADER_HPP

#include <string>
#include <fstream>

#include "../../Utility/uuid.hpp"
#include "../../Utility/moduleType.hpp"

enum class HeaderFieldType : uint8_t {
    HeaderSize    = 1,
    MetadataSize = 2,
    DataSize      = 3,
    StringSize    = 4,
    ModuleType    = 5,
    SchemaPath    = 6,
    Compression   = 7,
    Endianness    = 8,
    ModuleID      = 9
};

struct DataHeader {
protected:

    uint32_t headerSize = 0;
    uint64_t metaDataSize = 0;
    uint64_t dataSize = 0;
    uint64_t stringBufferSize = 0;

    uint64_t moduleStartOffset;
    uint64_t totalModuleSize = 0;
    
    std::streampos headerSizePos = 0;
    std::streampos metadataSizePos = 0;
    std::streampos dataSizePos = 0;
    std::streampos stringBufferSizePos = 0;

    std::streampos dataOffsetPos = 0;
    std::streampos stringOffsetPos = 0;

    ModuleType moduleType;
    std::string schemaPath;
    bool compression = false;
    bool littleEndian = true;
    UUID moduleID;

    void writeTLVString(std::ostream& out, HeaderFieldType type, const std::string& value) const;
    void writeTLVBool(std::ostream& out, HeaderFieldType type, bool value) const;
    std::streampos writeTLVFixed(std::ostream& out, HeaderFieldType type, const void* data, uint32_t size) const;
    
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

    bool getCompression() const { return compression; }

    bool getLittleEndian() const { return littleEndian; }
    void setLittleEndian(bool lE) { littleEndian = lE; }

    virtual uint64_t getAdditionalOffset() const { return 0; }
    virtual void seAdditionalOffset(uint64_t) {}

// METHODS
    virtual ~DataHeader() = default;

    void writeToFile(std::ostream& out);

    virtual void updateHeader(std::ostream& out);

    void readDataHeader(std::istream& in);

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