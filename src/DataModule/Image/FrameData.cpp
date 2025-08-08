#include "FrameData.hpp"
#include <cstring>

FrameData::FrameData(const std::string& schemaPath, UUID uuid)
    : DataModule(schemaPath, uuid, ModuleType::Frame)
{
    initialise(); // Parse the schema and set up metadata fields
}



void FrameData::writeData(std::ostream& out) const {
    // Write pixelData to stream
    if (!pixelData.empty()) {
        out.write(reinterpret_cast<const char*>(pixelData.data()), pixelData.size());
    }

    header->setDataSize(pixelData.size());

}

void FrameData::decodeData(std::istream& in, size_t actualDataSize) {
    pixelData.resize(actualDataSize);
    in.read(reinterpret_cast<char*>(pixelData.data()), actualDataSize);
}

void FrameData::printData(std::ostream& out) const {
    out << "FrameData with " << pixelData.size() << " bytes of pixel data.\n";
}

// Override the virtual method for frame-specific data
std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> 
FrameData::getModuleSpecificData() const {
    return pixelData; // Return the pixel data as std::vector<uint8_t>
}

// Metadata methods
size_t FrameData::getMetadataSize() const {
    // Calculate metadata size based on the actual metadata rows (same as base class)
    return metaDataRows.size() * metaDataRowSize;
}

void FrameData::writeMetadata(std::ostream& out) {
    // Write metadata using the base class method
    writeMetaData(out);
}