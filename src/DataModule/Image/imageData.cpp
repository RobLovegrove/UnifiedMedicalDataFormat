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

void ImageData::addData(const nlohmann::json& data) {}
void ImageData::decodeData(std::istream& in, size_t actualDataSize) {}
void ImageData::printData(std::ostream& out) const {}


        