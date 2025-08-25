#ifndef TLV_HEADER_HPP
#define TLV_HEADER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <expected>

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
    ModuleID = 20,
    EncounterSize = 21,
    LinkSize = 22
};

void writeTLVString(std::ostream& out, HeaderFieldType type, const std::string& value);
void writeTLVBool(std::ostream& out, HeaderFieldType type, bool value);
std::streampos writeTLVFixed(std::ostream& out, HeaderFieldType type, const void* data, uint32_t size);

std::expected<std::streampos, std::string> findTLVOffset(std::fstream& fileStream, HeaderFieldType type, uint32_t headerSize);

#endif // TLV_HEADER_HPP