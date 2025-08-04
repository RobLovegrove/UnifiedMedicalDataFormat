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
    header->setDataSize(rows.size() * rowSize);
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
    header->updateHeader(out, static_cast<uint64_t>(stringBufferStart), static_cast<uint64_t>(iamgeDataStart));
    out.seekp(moduleEnd);

    // Update XrefTable
    xref.addEntry(header->getModuleType(), header->getModuleID(), moduleStart, totalModuleSize);

}


std::unique_ptr<ImageData> ImageData::fromStream(
    std::istream& in, uint64_t moduleStartOffset, uint64_t moduleSize) {
    unique_ptr<ImageHeader> dmHeader = make_unique<ImageHeader>();
    dmHeader->readDataHeader(in);

    unique_ptr<ImageData> dm = make_unique<ImageData>(
        dmHeader->getSchemaPath(), dmHeader->getModuleID(), dmHeader->getModuleType());

    dm->header = std::move(dmHeader);
    dm->header->setModuleStartOffset(moduleStartOffset);

    cout << *(dm->header) << endl;

    uint64_t relativeStringOffset = dm->header->getStringOffset() - moduleStartOffset;
    uint64_t currentPos = in.tellg();
    
    in.seekg(relativeStringOffset);

    // Read in string buffer
    // size_t stringBufferSize = moduleSize - dm->header->getHeaderSize() - dm->header->getDataSize();
    
    size_t stringBufferSize = dm->header->getAdditionalOffset() - dm->header->getStringOffset();
    dm->stringBuffer.readFromFile(in, stringBufferSize);

    // Return back to start of data
    in.seekg(currentPos);

    // Read in data
    if (dm->header->getCompression()) {
        cout << "TODO: Handle file compression" << endl;
    } else {
        dm->decodeRows(in, dm->header->getDataSize());
    }

    // Read in image data
    // relativeOffset = dm->header->imageOffset - moduleStartOffset;
    // in.seekg(relativeOffset);


    return dm;
} 


        