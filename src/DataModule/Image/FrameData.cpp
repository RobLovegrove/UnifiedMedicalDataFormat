#include "FrameData.hpp"
#include <cstring>

FrameData::FrameData(const std::string& schemaPath, UUID uuid)
    : DataModule(schemaPath, uuid, ModuleType::Image) // or ModuleType::Frame if you add it
{
    std::cout << "FrameData constructor called with schema: " << schemaPath << std::endl;
    // Additional initialization if needed
}



std::streampos FrameData::writeData(std::ostream& out) const {
    // Write pixelData to stream
    if (!pixelData.empty()) {
        out.write(reinterpret_cast<const char*>(pixelData.data()), pixelData.size());
    }
    return out.tellp();
}

void FrameData::decodeData(std::istream& in, size_t actualDataSize) {

    std::cout << "=== FrameData::decodeData called with dataSize: " << actualDataSize << " ===" << std::endl;
    pixelData.resize(actualDataSize);
    in.read(reinterpret_cast<char*>(pixelData.data()), actualDataSize);
}

void FrameData::printData(std::ostream& out) const {
    out << "FrameData with " << pixelData.size() << " bytes of pixel data.\n";
}