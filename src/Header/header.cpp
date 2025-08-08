#include "header.hpp"
#include "nlohmann/json.hpp"
#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"

#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdio>
#include <chrono>
#include <iomanip>

using namespace std;

bool Header::writePrimaryHeader(ofstream& outfile, XRefTable& xref) {

    uint64_t offset = 0;
    if (!getCurrentFilePosition(outfile, offset)) { return false; };

    uint32_t size = MAGIC_NUMBER.size();

    // WRITE MAGIC NUMBER
    outfile.write(MAGIC_NUMBER.data(), size);

    // ADD HEADER TO XREFTABLE 
    xref.addEntry(ModuleType::FileHeader, UUID(), offset, size);

    return outfile.good();
}

bool Header::readPrimaryHeader(std::ifstream& inFile) {

    // Confirm correct magic number and version
    string magicLine;
    getline(inFile, magicLine);

    if (!magicLine.starts_with("#UMDFv")) return false;

    Version version;
    try {   
        version = Version::parse(magicLine.substr(6));
    } catch (const exception& e) {
        cerr << "Failed to parse version: " << e.what() << endl;
        return false;
    }

    if (version.major > UMDF_VERSION.major) {
        cerr << "Unsupported UMDF version: " << version.toString()
              << " (tool supports up to " << UMDF_VERSION.toString() << ")" << endl;
        return false;
    }

    return true;
}