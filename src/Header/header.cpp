#include "header.hpp"
#include "nlohmann/json.hpp"
#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"
#include "DataModule/Header/dataHeader.hpp"

#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdio>
#include <chrono>
#include <iomanip>

using namespace std;

bool Header::writePrimaryHeader(ostream& outfile) {

    uint64_t offset = 0;
    if (!getCurrentFilePosition(outfile, offset)) { return false; };

    uint32_t size = MAGIC_NUMBER.size();

    // WRITE MAGIC NUMBER
    outfile.write(MAGIC_NUMBER.data(), size);

    // Get current file position
    streampos startPosition = outfile.tellp();

    // WRITE HEADER SIZE
    uint32_t headerSize = 0;
    streampos headerSizeOffset = writeTLVFixed(
        outfile, 
        HeaderFieldType::HeaderSize, 
        &headerSize, 
        sizeof(uint32_t));

    // WRITE ENCRYPTION HEADER
    if (encryptionData.encryptionType == EncryptionType::NONE) {
        writeTLVFixed(
            outfile, HeaderFieldType::EncryptionType, 
            &encryptionData.encryptionType, 
            sizeof(uint8_t));
    }
    else {
        writeTLVFixed(
            outfile, HeaderFieldType::EncryptionType,
            &encryptionData.encryptionType, 
            sizeof(uint8_t));

        writeTLVFixed(
            outfile, 
            HeaderFieldType::BaseSalt, 
            encryptionData.baseSalt.data(), 
            encryptionData.baseSalt.size());

        writeTLVFixed(
            outfile, 
            HeaderFieldType::MemoryCost, 
            &encryptionData.memoryCost, 
            sizeof(uint64_t));

        writeTLVFixed(
            outfile, 
            HeaderFieldType::TimeCost, 
            &encryptionData.timeCost, 
            sizeof(uint32_t));

        writeTLVFixed(
            outfile, 
            HeaderFieldType::Parallelism, 
            &encryptionData.parallelism, 
            sizeof(uint32_t));
    }

    // Calculate header size
    streampos endPosition = outfile.tellp();
    headerSize = static_cast<uint32_t>(endPosition - startPosition);

    // Update header size
    outfile.seekp(headerSizeOffset);
    outfile.write(reinterpret_cast<const char*>(&headerSize), sizeof(headerSize));

    return outfile.good();
}

std::expected<EncryptionData, std::string> Header::readPrimaryHeader(std::istream& inFile) {

    // Confirm correct magic number and version
    string magicLine;
    getline(inFile, magicLine);

    if (!magicLine.starts_with("#UMDFv")) return std::unexpected("Invalid magic number");

    Version version;
    try {   
        version = Version::parse(magicLine.substr(6));
    } catch (const exception& e) {
        return std::unexpected("Failed to parse version");
    }

    if (version.major > UMDF_VERSION.major) {
        return std::unexpected("Unsupported UMDF version");
    }

    // Read header size
    uint8_t typeId;
    uint32_t length;

    inFile.read(reinterpret_cast<char*>(&typeId), sizeof(typeId));
    inFile.read(reinterpret_cast<char*>(&length), sizeof(length));


    if (typeId != static_cast<uint8_t>(HeaderFieldType::HeaderSize)) {
        return std::unexpected("Invalid header: expected HeaderSize after magic number.");
    }

    uint32_t headerSize;
    inFile.read(reinterpret_cast<char*>(&headerSize), sizeof(headerSize));

    size_t bytesRead = sizeof(typeId) + sizeof(length) + sizeof(headerSize);    
    while (bytesRead < headerSize) {

        inFile.read(reinterpret_cast<char*>(&typeId), sizeof(typeId));
        inFile.read(reinterpret_cast<char*>(&length), sizeof(length));
        bytesRead += sizeof(typeId) + sizeof(length);

        std::vector<char> buffer(length);
        inFile.read(buffer.data(), length);
        bytesRead += length;

        auto type = static_cast<HeaderFieldType>(typeId);
        switch (type) {
            case HeaderFieldType::EncryptionType:
                if (length != 1) return std::unexpected("Invalid EncryptionType length.");
                encryptionData.encryptionType = EncryptionManager::decodeEncryptionType(buffer[0]);
                break;

            case HeaderFieldType::BaseSalt:
                encryptionData.baseSalt = std::vector<uint8_t>(buffer.data(), buffer.data() + length);
                break;

            case HeaderFieldType::MemoryCost:
                if (length != sizeof(uint64_t)) return std::unexpected("Invalid EncryptionMemoryCost length.");
                std::memcpy(&encryptionData.memoryCost, buffer.data(), sizeof(uint64_t));
                break;

            case HeaderFieldType::TimeCost:
                if (length != sizeof(uint32_t)) return std::unexpected("Invalid EncryptionTimeCost length.");
                std::memcpy(&encryptionData.timeCost, buffer.data(), sizeof(uint32_t));
                break;

            case HeaderFieldType::Parallelism:
                if (length != sizeof(uint32_t)) return std::unexpected("Invalid EncryptionParallelism length.");
                std::memcpy(&encryptionData.parallelism, buffer.data(), sizeof(uint32_t));
                break;
            
            default:
                cout << "Unknown HeaderFieldType: " << typeId << endl;
                cout << "Length: " << length << endl;
                return std::unexpected("Unknown HeaderFieldType: " + std::to_string(typeId));
        }
    }

    if (bytesRead != headerSize) {
        return std::unexpected("Invalid header size");
    }

    return encryptionData;
}

std::streampos Header::writeTLVFixed(std::ostream& out, HeaderFieldType type, const void* data, uint32_t size) const {
    uint8_t typeID = static_cast<uint8_t>(type);
    out.write(reinterpret_cast<const char*>(&typeID), sizeof(typeID));
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    std::streampos pos = out.tellp();
    out.write(reinterpret_cast<const char*>(data), size);
    return pos;
}