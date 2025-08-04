#include "dataHeader.hpp"
#include "imageHeader.hpp"

#include <string>
#include <stdexcept>
#include <iostream>


std::unique_ptr<DataHeader> DataHeader::create(ModuleType type) {
    if (type == ModuleType::Image) {
        return std::make_unique<ImageHeader>();
    } 
    else if (type == ModuleType::Tabular) {
        return std::make_unique<DataHeader>();
    }
    else {
        std::cerr << "TODO: Handle unknown moduleType when creating data header" 
        << std::endl;
    }
}


/* ================ WRTIE FUNCTIONS ================ */

void DataHeader::writeToFile(std::ostream& out) {
    // Remember where the header starts (before the TLVs)
    std::streampos headerStart = out.tellp();

    // Write placeholder for header size
    uint32_t placeholderHeaderSize = 0;
    std::streampos headerSizePos = out.tellp();  // Save this to seek back later
    writeTLVFixed(out, HeaderFieldType::HeaderSize, &placeholderHeaderSize, sizeof(placeholderHeaderSize));


    dataSizePos = writeTLVFixed(out, HeaderFieldType::DataSize, &dataSize, sizeof(dataSize)); // Updated after write

    stringOffsetPos = writeTLVFixed(out, HeaderFieldType::StringBufferOffset, &stringOffset, sizeof(stringOffset)); // Updated after write

    writeAdditionalOffsets(out);

    writeTLVString(out, HeaderFieldType::ModuleType, module_type_to_string(moduleType));
    writeTLVString(out, HeaderFieldType::SchemaPath, schemaPath);
    writeTLVBool(out, HeaderFieldType::Compression, compression);
    writeTLVBool(out, HeaderFieldType::Endianness, littleEndian);

    const auto& uuidBytes = moduleID.data();
    writeTLVFixed(out, HeaderFieldType::ModuleID, uuidBytes.data(), uuidBytes.size());

    // Calculate how many bytes we wrote for the header
    std::streampos headerEnd = out.tellp();
    uint32_t actualHeaderSize = static_cast<uint32_t>(headerEnd - headerStart);

    std::cout << "The header is: " << actualHeaderSize << " bytes long." << std::endl;

    // Seek back and overwrite the value of the header size field
    out.seekp(headerSizePos + static_cast<std::streamoff>(sizeof(uint8_t) + sizeof(uint32_t)));
    out.write(reinterpret_cast<const char*>(&actualHeaderSize), sizeof(actualHeaderSize));

    // Return write position to end
    out.seekp(headerEnd);
}

void DataHeader::updateHeader(std::ostream& out, std::uint32_t size, uint64_t stringOffset) {

    dataSize = size;
    if (dataSizePos == 0) { throw std::runtime_error("Failed to update dataSize"); }
    
    out.seekp(dataSizePos);
    out.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));

    // Seek to stringOffset value start
    out.seekp(stringOffsetPos);
    // Write stringOffset value (8 bytes)
    out.write(reinterpret_cast<const char*>(&stringOffset), sizeof(stringOffset));

}

void DataHeader::updateHeader(std::ostream& out, std::uint32_t size, uint64_t stringOffset, uint64_t) {
    // Optional fallback behavior — e.g., just call the 3-arg version
    updateHeader(out, size, stringOffset);
}

void DataHeader::writeTLVString(
    std::ostream& out, HeaderFieldType type, const std::string& value) const {

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

std::streampos DataHeader::writeTLVFixed(std::ostream& out, HeaderFieldType type, const void* data, uint32_t size) const {
    uint8_t typeID = static_cast<uint8_t>(type);
    out.write(reinterpret_cast<const char*>(&typeID), sizeof(typeID));
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    std::streampos pos = out.tellp();
    out.write(reinterpret_cast<const char*>(data), size);
    return pos;
}


/* ================ READ FUNCTIONS ================ */

void DataHeader::readDataHeader(std::istream& in) {

    // Read header size
    uint8_t typeId;
    uint32_t length;
    in.read(reinterpret_cast<char*>(&typeId), sizeof(typeId));
    in.read(reinterpret_cast<char*>(&length), sizeof(length));

    if (typeId != static_cast<uint8_t>(HeaderFieldType::HeaderSize)) {
        throw std::runtime_error("Invalid header: expected HeaderSize first.");
    }

    in.read(reinterpret_cast<char*>(&headerSize), sizeof(headerSize));

    size_t bytesRead = sizeof(typeId) + sizeof(length) + sizeof(headerSize);    
    while (bytesRead < headerSize) {
        in.read(reinterpret_cast<char*>(&typeId), sizeof(typeId));
        in.read(reinterpret_cast<char*>(&length), sizeof(length));
        bytesRead += sizeof(typeId) + sizeof(length);

        std::vector<char> buffer(length);
        in.read(buffer.data(), length);
        bytesRead += length;

        auto type = static_cast<HeaderFieldType>(typeId);
        switch (type) {
            case HeaderFieldType::DataSize:
                if (length != sizeof(dataSize)) throw std::runtime_error("Invalid DataSize length.");
                std::memcpy(&dataSize, buffer.data(), sizeof(dataSize));
                break;

            case HeaderFieldType::StringBufferOffset:
                if (length != sizeof(stringOffset)) throw std::runtime_error("Invalid stringOffset.");
                std::memcpy(&stringOffset, buffer.data(), sizeof(stringOffset));
                break;

            case HeaderFieldType::ModuleType:
                moduleType = module_type_from_string(std::string(buffer.data(), length));
                break;

            case HeaderFieldType::SchemaPath:
                schemaPath = std::string(buffer.data(), length);
                break;

            case HeaderFieldType::Compression:
                if (length != 1) throw std::runtime_error("Invalid Compression length.");
                compression = buffer[0] != 0;
                break;

            case HeaderFieldType::Endianness:
                if (length != 1) throw std::runtime_error("Invalid Endianness length.");
                littleEndian = buffer[0] != 0;
                break;

            case HeaderFieldType::ModuleID:
                if (length != 16) throw std::runtime_error("Invalid UUID length.");
                std::array<uint8_t, 16> id;
                std::memcpy(id.data(), buffer.data(), 16);
                moduleID.setData(id);
                break;

            default:
                if (!handleExtraField(type, buffer)) {
                    throw std::runtime_error("Unknown HeaderFieldType: " + std::to_string(typeId));
                }
        }
    }

    if (bytesRead != headerSize) {
        throw std::runtime_error("Header read mismatch.");
    }

}

std::ostream& operator<<(std::ostream& os, const DataHeader& header) {
    os << "DataHeader {\n"
       << "  headerSize   : " << header.headerSize << "\n"
       << "  dataSize     : " << header.dataSize << "\n"
       << "  stringOffset : " << header.stringOffset << "\n"
       << header.outputAdditionalOffsets()
       << "  moduleType   : " << header.moduleType << "\n"
       << "  schemaPath   : " << header.schemaPath << "\n"
       << "  compression  : " << std::boolalpha << header.compression << "\n"
       << "  littleEndian : " << std::boolalpha << header.littleEndian << "\n"
       << "  moduleID     : " << header.moduleID.toString() << "\n"
       << "}";

    return os;
}