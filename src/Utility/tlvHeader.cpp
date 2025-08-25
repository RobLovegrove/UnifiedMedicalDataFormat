#include "tlvHeader.hpp"

std::expected<std::streampos, std::string> findTLVOffset(std::fstream& fileStream, HeaderFieldType type, uint32_t headerSize) {
    // Read through the header to find the CurrentFlag TLV
    size_t bytesRead = 0;
    uint8_t typeId;
    uint32_t length;

    while (bytesRead < headerSize) {
        fileStream.read(reinterpret_cast<char*>(&typeId), sizeof(typeId));
        fileStream.read(reinterpret_cast<char*>(&length), sizeof(length));
        bytesRead += sizeof(typeId) + sizeof(length);

        if (typeId == static_cast<uint8_t>(type)) {
            return fileStream.tellg();
        }
        else {
            // Skip the TLV
            fileStream.seekg(length, std::ios::cur);
            bytesRead += length;
        }
    }

    return std::unexpected("TLV field not found");
}

void writeTLVString(std::ostream& out, HeaderFieldType type, const std::string& value) {

    uint8_t typeID = static_cast<uint8_t>(type);
    uint32_t len = static_cast<uint32_t>(value.size());
    out.write(reinterpret_cast<const char*>(&typeID), sizeof(typeID));
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(value.data(), len);
}

void writeTLVBool(std::ostream& out, HeaderFieldType type, bool value) {
    uint8_t v = value ? 1 : 0;
    writeTLVFixed(out, type, &v, sizeof(v));
}

std::streampos writeTLVFixed(std::ostream& out, HeaderFieldType type, const void* data, uint32_t size) {
    uint8_t typeID = static_cast<uint8_t>(type);
    out.write(reinterpret_cast<const char*>(&typeID), sizeof(typeID));
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    std::streampos pos = out.tellp();
    out.write(reinterpret_cast<const char*>(data), size);
    return pos;
}