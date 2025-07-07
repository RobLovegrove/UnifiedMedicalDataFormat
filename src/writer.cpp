#include "writer.hpp"
#include "Header/header.hpp"
#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"
#include "Patient/patient.hpp"

#include <nlohmann/json.hpp> 

#include <fstream>
#include <cstdint>
#include <iostream>

using namespace std;

bool Writer::writeNewFile(const string& filename, const Patient& patient) {

    // OPEN FILE
    ofstream outfile(filename, ios::binary);
    if (!outfile) return false;

    // CREATE HEADER
    if (!header.writePrimaryHeader(outfile, xref)) return false;

    // CREATE PATIENT DATA
    if (!patient.writeToFile(outfile, xref)) return false;

    // APPEND XREF
    if (!writeXref(outfile)) return false;

    // CLOSE FILE
    outfile.close();
    return true;
}

void Writer::setFileAccessMode(FileAccessMode mode) {
    accessMode = mode;
}

bool Writer::writeXref(std::ofstream& outfile) { 
    uint64_t offset = 0;
    if (!getCurrentFilePosition(outfile, offset)) { return false; };
    xref.setXrefOffset(offset);
    return xref.writeXref(outfile);
}