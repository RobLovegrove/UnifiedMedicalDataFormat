#include "imageHeader.hpp"

#include <iostream>
#include <sstream>

using namespace std;

void ImageHeader::writeAdditionalOffsets(ostream& out) {

    imageOffsetPos = writeTLVFixed(out, HeaderFieldType::ImageDataOffset, &imageOffset, sizeof(imageOffset)); // Updated after write

}

std::string ImageHeader::outputAdditionalOffsets() const {

    std::ostringstream oss;
    oss << "  imageOffset  : " << imageOffset << "\n";
    return oss.str();

}

bool ImageHeader::handleExtraField(HeaderFieldType type, const std::vector<char>& buffer) {

    if (type == HeaderFieldType::ImageDataOffset) {
        if (buffer.size() != sizeof(imageOffset)) {
            throw std::runtime_error("Invalid ImageOffset size");
        }
        std::memcpy(&imageOffset, buffer.data(), sizeof(imageOffset));
        return true;
    }
    return false;
}

void ImageHeader::updateHeader(
    std::ostream& out, uint64_t stringOffset, uint64_t imageOffset) {
    
    DataHeader::updateHeader(out, stringOffset);

    // Seek to stringOffset value start
    out.seekp(imageOffsetPos);
    // Write stringOffset value (8 bytes)
    out.write(reinterpret_cast<const char*>(&imageOffset), sizeof(imageOffset));

}