#include "imageData.hpp"
#include "../../Utility/uuid.hpp"
#include "../Header/dataHeader.hpp"
#include "../Header/imageHeader.hpp"

#include "string"
#include <fstream>
#include <iostream>

using namespace std;

void ImageData::writeBinary(std::ostream& out, XRefTable& xref) {

    streampos moduleStart = out.tellp();

    // Write header
    header->writeToFile(out);

    // Write Data
    writeTabularData(out);

    streampos stringBufferStart = out.tellp();

    // Write String Buffer
    writeStringBuffer(out);

    streampos iamgeDataStart = out.tellp();

    // Write Image Data
    if (!rawImageData.empty()) {
        out.write(reinterpret_cast<const char*>(rawImageData.data()), rawImageData.size());
    }

    streampos moduleEnd = out.tellp();

    uint32_t totalModuleSize = static_cast<uint32_t>(moduleEnd - moduleStart);

    // Update Header
    header->updateHeader(out, totalModuleSize, static_cast<uint64_t>(stringBufferStart), static_cast<uint64_t>(iamgeDataStart));
    out.seekp(moduleEnd);

    // Update XrefTable
    xref.addEntry(header->moduleType, header->moduleID, moduleStart, totalModuleSize);

}


std::unique_ptr<ImageData> ImageData::fromStream(
    std::istream& in, uint64_t moduleStartOffset) {
    unique_ptr<DataHeader> dmHeader = make_unique<ImageHeader>();
    dmHeader->readDataHeader(in);

    unique_ptr<ImageData> dm = make_unique<ImageData>(
        dmHeader->schemaPath, dmHeader->moduleID, dmHeader->moduleType);

    dm->header = std::move(dmHeader);
    dm->header->moduleStartOffset = moduleStartOffset;
} 


        