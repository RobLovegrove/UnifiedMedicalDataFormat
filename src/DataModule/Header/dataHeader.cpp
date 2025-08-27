#include "dataHeader.hpp"
#include "../../Utility/Compression/CompressionType.hpp"
#include "../../Utility/Encryption/encryptionManager.hpp"
#include "../../Utility/tlvHeader.hpp"

#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <expected>

/* ================ WRTIE FUNCTIONS ================ */

void DataHeader::writeToFile(std::ostream& out) {
    // Remember where the header starts (before the TLVs)
    std::streampos headerStart = out.tellp();

    // writeTLVFixed returns the position of the Value in the TLV flag
    headerSizePos = writeTLVFixed(out, HeaderFieldType::HeaderSize, &headerSize, sizeof(headerSize)); 
    stringBufferSizePos = writeTLVFixed(out, HeaderFieldType::StringSize, &stringBufferSize, sizeof(stringBufferSize));
    metadataSizePos = writeTLVFixed(out, HeaderFieldType::MetadataSize, &metaDataSize, sizeof(metaDataSize));
    dataSizePos = writeTLVFixed(out, HeaderFieldType::DataSize, &dataSize, sizeof(dataSize)); 

    writeTLVBool(out, HeaderFieldType::IsCurrent, isCurrent);
    writeTLVFixed(out, HeaderFieldType::PreviousVersion, &previousVersion, sizeof(previousVersion));

    writeTLVString(out, HeaderFieldType::ModuleType, module_type_to_string(moduleType));
    writeTLVString(out, HeaderFieldType::SchemaPath, schemaPath);

    uint8_t metadataCompressionValue = encodeCompression(metadataCompression);
    uint8_t dataCompressionValue = encodeCompression(dataCompression);

    writeTLVFixed(out, HeaderFieldType::MetadataCompression, &metadataCompressionValue, sizeof(metadataCompressionValue));
    writeTLVFixed(out, HeaderFieldType::DataCompression, &dataCompressionValue, sizeof(dataCompressionValue));

    if (encryptionData.encryptionType != EncryptionType::NONE) {

        encryptionData.moduleSalt = EncryptionManager::generateSalt(16);  // 16 bytes
        encryptionData.iv = EncryptionManager::generateIV(12);   

        writeTLVFixed(out, HeaderFieldType::ModuleSalt, encryptionData.moduleSalt.data(), encryptionData.moduleSalt.size());
        writeTLVFixed(out, HeaderFieldType::IV, encryptionData.iv.data(), encryptionData.iv.size());
        authTagPos = writeTLVFixed(out, HeaderFieldType::AuthTag, encryptionData.authTag.data(), crypto_aead_aes256gcm_ABYTES);
    }

    writeTLVBool(out, HeaderFieldType::Endianness, littleEndian);

    const auto& uuidBytes = moduleID.data();
    writeTLVFixed(out, HeaderFieldType::ModuleID, uuidBytes.data(), uuidBytes.size());
    int64_t timestamp = createdAt.getTimestamp(); 
    writeTLVFixed(out, HeaderFieldType::CreatedAt, &timestamp, sizeof(timestamp));
    writeTLVString(out, HeaderFieldType::CreatedBy, createdBy);
    timestamp = modifiedAt.getTimestamp();
    writeTLVFixed(out, HeaderFieldType::ModifiedAt, &timestamp, sizeof(timestamp));
    writeTLVString(out, HeaderFieldType::ModifiedBy, modifiedBy);

    // Calculate how many bytes we wrote for the header
    std::streampos headerEnd = out.tellp();
    headerSize = static_cast<uint32_t>(headerEnd - headerStart); // Will update in updateHeaderSize

}

bool DataHeader::updateIsCurrent(bool newIsCurrent, std::fstream& fileStream) {

    // Save current put position
    std::streampos currentPos = fileStream.tellp();
    
    // Flush any pending operations
    fileStream.flush();
    
    // Find TLV position (read-only operation)
    auto offset = findTLVOffset(fileStream, HeaderFieldType::IsCurrent, headerSize);
    if (!offset) return false;
    
    // Flush again before writing
    fileStream.flush();
    
    // Seek to exact position and write
    fileStream.seekp(offset.value(), std::ios::beg);
    fileStream.write(reinterpret_cast<const char*>(&newIsCurrent), sizeof(newIsCurrent));
    
    // Flush the write immediately
    fileStream.flush();
    
    // Restore put position
    fileStream.seekp(currentPos);
    
    return fileStream.good();
}

void DataHeader::updateHeader(std::ostream& out) {

    // Update header size
    out.seekp(headerSizePos);
    out.write(reinterpret_cast<const char*>(&headerSize), sizeof(headerSize));

    // Update string buffer size
    out.seekp(stringBufferSizePos);
    out.write(reinterpret_cast<const char*>(&stringBufferSize), sizeof(stringBufferSize));
    
    // Update metadata size
    out.seekp(metadataSizePos);
    out.write(reinterpret_cast<const char*>(&metaDataSize), sizeof(metaDataSize));

    // Update data size
    out.seekp(dataSizePos);
    out.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));

    if (encryptionData.encryptionType != EncryptionType::NONE) {

        // Update auth tag
        out.seekp(authTagPos);
        out.write(reinterpret_cast<const char*>(encryptionData.authTag.data()), encryptionData.authTag.size());
    }

}



/* ================ READ FUNCTIONS ================ */


void DataHeader::readHeaderSize(std::istream& in) {
    // Read header size
    uint8_t typeId;
    uint32_t length;
    in.read(reinterpret_cast<char*>(&typeId), sizeof(typeId));
    in.read(reinterpret_cast<char*>(&length), sizeof(length));

    if (typeId != static_cast<uint8_t>(HeaderFieldType::HeaderSize)) {
        throw std::runtime_error("Invalid header: expected HeaderSize first.");
    }

    in.read(reinterpret_cast<char*>(&headerSize), sizeof(headerSize));
}

void DataHeader::readDataHeader(std::istream& in) {

    uint8_t typeId;
    uint32_t length;

    readHeaderSize(in);

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
            case HeaderFieldType::MetadataSize:
                if (length != sizeof(metaDataSize)) throw std::runtime_error("Invalid DataSize length.");
                std::memcpy(&metaDataSize, buffer.data(), sizeof(metaDataSize));
                break;

            case HeaderFieldType::DataSize:
                if (length != sizeof(dataSize)) throw std::runtime_error("Invalid DataSize length.");
                std::memcpy(&dataSize, buffer.data(), sizeof(dataSize));
                break;

            case HeaderFieldType::StringSize:
                if (length != sizeof(stringBufferSize)) throw std::runtime_error("Invalid StringSize length.");
                std::memcpy(&stringBufferSize, buffer.data(), sizeof(stringBufferSize));
                break;

            case HeaderFieldType::IsCurrent:
                if (length != sizeof(isCurrent)) throw std::runtime_error("Invalid IsCurrent length.");
                std::memcpy(&isCurrent, buffer.data(), sizeof(isCurrent));
                break;

            case HeaderFieldType::PreviousVersion:
                if (length != sizeof(previousVersion)) throw std::runtime_error("Invalid PreviousVersion length.");
                std::memcpy(&previousVersion, buffer.data(), sizeof(previousVersion));
                break;

            case HeaderFieldType::ModuleType:
                moduleType = module_type_from_string(std::string(buffer.data(), length));
                break;

            case HeaderFieldType::SchemaPath:
                schemaPath = std::string(buffer.data(), length);
                break;

            case HeaderFieldType::MetadataCompression:
                if (length != 1) throw std::runtime_error("Invalid MetadataCompression length.");
                metadataCompression = decodeCompressionType(buffer[0]);
                break;

            case HeaderFieldType::DataCompression:
                if (length != 1) throw std::runtime_error("Invalid DataCompression length.");
                dataCompression = decodeCompressionType(buffer[0]);
                break;

            case HeaderFieldType::ModuleSalt:
                encryptionData.moduleSalt = std::vector<uint8_t>(buffer.data(), buffer.data() + length);
                break;

            case HeaderFieldType::IV:
                encryptionData.iv = std::vector<uint8_t>(buffer.data(), buffer.data() + length);
                break;

            case HeaderFieldType::AuthTag:
                encryptionData.authTag = std::vector<uint8_t>(buffer.data(), buffer.data() + length);
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

            case HeaderFieldType::CreatedAt:
                if (length != sizeof(createdAt.getTimestamp())) {
                    throw std::runtime_error("Invalid CreatedAt length.");
                }
                uint64_t timestamp;
                std::memcpy(&timestamp, buffer.data(), sizeof(timestamp));
                createdAt = DateTime(timestamp);
                break;

            case HeaderFieldType::CreatedBy:
                createdBy = std::string(buffer.data(), length);
                break;

            case HeaderFieldType::ModifiedAt:
                if (length != sizeof(modifiedAt.getTimestamp())) {
                    throw std::runtime_error("Invalid ModifiedAt length.");
                }
                std::memcpy(&timestamp, buffer.data(), sizeof(timestamp));
                modifiedAt = DateTime(timestamp);
                break;

            case HeaderFieldType::ModifiedBy:
                modifiedBy = std::string(buffer.data(), length);
                break;

            default:
                throw std::runtime_error("Unknown HeaderFieldType: " + std::to_string(typeId));
        }
    }

    if (bytesRead != headerSize) {
        throw std::runtime_error("Header read mismatch.");
    }

}

uint64_t DataHeader::getModuleSize() const {
    if (totalModuleSize == 0) {
        return headerSize + metaDataSize + dataSize + stringBufferSize;
    } else {
        return totalModuleSize;
    }
}

std::ostream& operator<<(std::ostream& os, const DataHeader& header) {
    os << "DataHeader {\n"
       << "  headerSize          : " << header.headerSize << "\n"
       << "  stringBufferSize    : " << header.stringBufferSize << "\n"
       << "  metaDataSize        : " << header.metaDataSize << "\n"
       << "  dataSize            : " << header.dataSize << "\n"
       << "  isCurrent           : " << header.isCurrent << "\n"
       << "  previousVersion     : " << header.previousVersion << "\n"
       << "  moduleType          : " << header.moduleType << "\n"
       << "  schemaPath          : " << header.schemaPath << "\n"
       << "  metadataCompression : " << compressionToString(header.metadataCompression) << "\n"
       << "  dataCompression     : " << compressionToString(header.dataCompression) << "\n"
       << "  encryptionType      : "
       << EncryptionManager::encryptionToString(header.encryptionData.encryptionType) << "\n";
       if (header.encryptionData.encryptionType != EncryptionType::NONE) {
           os << "  baseSalt            : " << header.encryptionData.baseSalt.size() << "\n" 
              << "  moduleSalt          : " << header.encryptionData.moduleSalt.size() << "\n"
              << "  memoryCost          : " << header.encryptionData.memoryCost << "\n"
              << "  timeCost            : " << header.encryptionData.timeCost << "\n"
              << "  parallelism         : " << header.encryptionData.parallelism << "\n"
              << "  iv                  : " << header.encryptionData.iv.size() << "\n"
              << "  authTag             : " << header.encryptionData.authTag.size() << "\n";
       }
       os 
       << "  littleEndian        : " << std::boolalpha << header.littleEndian << "\n"
       << "  moduleID            : " << header.moduleID.toString() << "\n"
       << "  createdAt           : " << header.createdAt.toString() << "\n"
       << "  createdBy           : " << header.createdBy << "\n"
       << "  modifiedAt          : " << header.modifiedAt.toString() << "\n"
       << "  modifiedBy          : " << header.modifiedBy << "\n"
       << "}";

    return os;
}