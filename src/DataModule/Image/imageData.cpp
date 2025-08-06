#include "imageData.hpp"
#include "../../Utility/uuid.hpp"
#include "../Header/dataHeader.hpp"
#include "../dataModule.hpp"

#include "string"
#include <fstream>
#include <iostream>

using namespace std;

ImageData::ImageData(const string& schemaPath, UUID uuid) : DataModule(schemaPath, uuid, ModuleType::Image) {
    initialise();
}

streampos ImageData::writeData(std::ostream& out) {

    streampos pos = out.tellp();

    if (!rawImageData.empty()) {
        size_t size = rawImageData.size();
        header->setDataSize(size);
        out.write(reinterpret_cast<const char*>(rawImageData.data()), size);
    }

    return pos;
}

void ImageData::decodeData(std::istream& in, size_t actualDataSize) {
    rawImageData.resize(actualDataSize);
    in.read(reinterpret_cast<char*>(rawImageData.data()), actualDataSize);

    if (in.gcount() != static_cast<std::streamsize>(actualDataSize)) {
        throw std::runtime_error("Failed to read expected image data size.");
    }
}


void ImageData::printData(std::ostream& out) const {
    constexpr int width = 16;
    constexpr int height = 16;
    const std::string shades = " .:-=+*#%@";  // 10 levels of intensity

    if (rawImageData.size() < width * height) {
        out << "Image data incomplete or incorrect size.\n";
        return;
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t pixel = rawImageData[y * width + x];
            char shade = shades[pixel * shades.size() / 256];
            out << shade;
        }
        out << '\n';
    }

}


        