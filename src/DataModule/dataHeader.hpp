#ifndef DATAHEADER_HPP
#define DATAHEADER_HPP

#include <string>
#include <fstream>

#include "../Utility/uuid.hpp"

enum class HeaderFieldType : uint8_t {
    HeaderSize    = 1,
    DataSize      = 2,
    StringBufferOffset = 3,
    ModuleType    = 4,
    SchemaPath    = 5,
    Compression   = 6,
    Endianness    = 7,
    ModuleID      = 8
};

struct DataHeader {
    uint32_t headerSize = 0;
    uint64_t dataSize = 0;
    uint64_t stringOffset = 0;
    std::string moduleType;
    std::string schemaPath;
    bool compression = false;
    bool littleEndian = true;
    UUID moduleID;

    uint32_t dataSizePos = 0;
    uint64_t moduleStartOffset;

    void writeToFile(std::ostream& out);
    void updateHeader(std::ostream& out, std::uint32_t size, uint64_t stringOffset);

    void readDataHeader(std::istream& in);

    friend std::ostream& operator<<(std::ostream& os, const DataHeader& header);

private:
    void writeTLVString(std::ostream& out, HeaderFieldType type, const std::string& value) const;
    void writeTLVBool(std::ostream& out, HeaderFieldType type, bool value) const;
    void writeTLVFixed(std::ostream& out, HeaderFieldType type, const void* data, uint32_t size) const;
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