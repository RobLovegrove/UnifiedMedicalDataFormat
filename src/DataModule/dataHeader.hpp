#ifndef DATAHEADER_HPP
#define DATAHEADER_HPP

#include <string>
#include <fstream>

#include "../Utility/uuid.hpp"

enum class HeaderFieldType : uint8_t {
    HeaderSize    = 1,
    DataSize      = 2,
    ModuleType    = 3,
    SchemaPath    = 4,
    Compression   = 5,
    Endianness    = 6,
    ModuleID      = 7
};

struct DataHeader {
    uint32_t headerSize = 0;
    uint64_t dataSize = 0;
    std::string moduleType;
    std::string schemaPath;
    bool compression = false;
    bool littleEndian = true;
    UUID moduleID;
    uint32_t dataSizePos = 0;

    void writeToFile(std::ostream& out);
    void writeDataSize(std::ostream& out, std::uint32_t size);

private:
    void writeTLVString(std::ostream& out, HeaderFieldType type, const std::string& value) const;
    void writeTLVBool(std::ostream& out, HeaderFieldType type, bool value) const;
    void writeTLVFixed(std::ostream& out, HeaderFieldType type, const void* data, uint32_t size) const;
};


#endif