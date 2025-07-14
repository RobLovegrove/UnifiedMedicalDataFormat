#include "dataHeader.hpp"
#include <string>
#include <stdexcept>


void DataHeader::writeToFile(std::ostream& out) {
    // Remember where the header starts (before the TLVs)
    std::streampos headerStart = out.tellp();

    // Write placeholder for header size
    uint32_t placeholderHeaderSize = 0;
    std::streampos headerSizePos = out.tellp();  // Save this to seek back later
    writeTLVFixed(out, HeaderFieldType::HeaderSize, &placeholderHeaderSize, sizeof(placeholderHeaderSize));

    dataSizePos = out.tellp();

    // Write all the remaining fields
    writeTLVFixed(out, HeaderFieldType::DataSize, &dataSize, sizeof(dataSize));
    writeTLVString(out, HeaderFieldType::ModuleType, moduleType);
    writeTLVString(out, HeaderFieldType::SchemaPath, schemaPath);
    writeTLVBool(out, HeaderFieldType::Compression, compression);
    writeTLVBool(out, HeaderFieldType::Endianness, littleEndian);

    const auto& uuidBytes = moduleID.data();
    writeTLVFixed(out, HeaderFieldType::ModuleID, uuidBytes.data(), uuidBytes.size());

    // Calculate how many bytes we wrote for the header
    std::streampos headerEnd = out.tellp();
    uint32_t actualHeaderSize = static_cast<uint32_t>(headerEnd - headerStart);

    // Seek back and overwrite the value of the header size field
    out.seekp(headerSizePos + static_cast<std::streamoff>(sizeof(uint8_t) + sizeof(uint32_t)));
    out.write(reinterpret_cast<const char*>(&actualHeaderSize), sizeof(actualHeaderSize));

    // Return write position to end
    out.seekp(headerEnd);
}

void DataHeader::writeDataSize(std::ostream& out, std::uint32_t size) {

    dataSize = size;
    if (dataSizePos == 0) { throw std::runtime_error("Failed to update dataSize"); }
    
    out.seekp(dataSizePos + static_cast<std::streamoff>(sizeof(uint8_t) + sizeof(uint32_t)));
    out.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));

}

void DataHeader::writeTLVString(std::ostream& out, HeaderFieldType type, const std::string& value) const {
    uint8_t typeID = static_cast<uint8_t>(type);
    uint32_t len = static_cast<uint32_t>(value.size());
    out.write(reinterpret_cast<const char*>(&typeID), sizeof(typeID));
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(value.data(), len);
}

void DataHeader::writeTLVBool(std::ostream& out, HeaderFieldType type, bool value) const {
    uint8_t v = value ? 1 : 0;
    writeTLVFixed(out, type, &v, sizeof(v));
}

void DataHeader::writeTLVFixed(std::ostream& out, HeaderFieldType type, const void* data, uint32_t size) const {
    uint8_t typeID = static_cast<uint8_t>(type);
    out.write(reinterpret_cast<const char*>(&typeID), sizeof(typeID));
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    out.write(reinterpret_cast<const char*>(data), size);
}
