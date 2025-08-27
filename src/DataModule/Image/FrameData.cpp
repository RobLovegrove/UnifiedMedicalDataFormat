#include "FrameData.hpp"
#include <cstring>

FrameData::FrameData(const std::string& schemaPath, DataHeader& dataheader) : DataModule(schemaPath, dataheader) {
    // Set the encryption header to NONE
    EncryptionData encryptionData = header->getEncryptionData();    
    encryptionData.encryptionType = EncryptionType::NONE;
    header->setEncryptionData(encryptionData);
    
    initialise();
}


void FrameData::writeData(std::ostream& out) const {
    // Write pixelData to stream
    if (!pixelData.empty()) {
        out.write(reinterpret_cast<const char*>(pixelData.data()), pixelData.size());
    }

    header->setDataSize(pixelData.size());

}

void FrameData::readData(std::istream& in) {

    size_t size = header->getDataSize();
    pixelData.resize(size);
    in.read(reinterpret_cast<char*>(pixelData.data()), size);
    // Note: needsDecompression flag will be set by ImageData based on encoding
}

void FrameData::printData(std::ostream&) const {

}

void FrameData::addData(
    const std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>>& data) {

    if (std::holds_alternative<std::vector<uint8_t>>(data)) {
        pixelData = std::get<std::vector<uint8_t>>(data);
    }
}

// Override the virtual method for frame-specific data
std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
FrameData::getModuleSpecificData() const {
    return pixelData; // Return the pixel data as std::vector<uint8_t>
}