#include "writer.hpp"
#include "Header/header.hpp"
#include "Utility/utils.hpp"
#include "Patient/patient.hpp"

#include <nlohmann/json.hpp> 

#include <fstream>
#include <cstdint>
#include <iostream>

using namespace std;

bool Writer::writeNewFile(const string& filename, const Patient patient, const string& patientData) {

    // OPEN FILE
    ofstream outfile(filename, ios::binary);
    if (!outfile) return false;

    // CREATE HEADER
    if (!writeHeader(outfile)) return false;

    // CREATE PATIENT DATA
    if (!writePatientData(outfile)) return false;

    // Write data
    uint64_t offset = 0;
    if (!getCurrentFilePosition(outfile, offset)) { return false; };
    
    uint32_t dataLength = static_cast<uint32_t>(patientData.size());

    xref.addEntry(ModuleType::Patient, offset, dataLength);

    cout << "Data length: " << dataLength << endl;
    outfile.write(patientData.data(), dataLength);

    // APPEND XREF
    if (!writeXref(outfile)) return false;

    // CLOSE FILE
    outfile.close();
    return true;
}

void Writer::setFileAccessMode(FileAccessMode mode) {
    accessMode = mode;
}

bool Writer::writeHeader(ofstream& outfile) {
    return header.writePrimaryHeader(outfile, xref);
}

bool Writer::writePatientData(ofstream& outfile) {
    return patient.writeToFile(outfile, xref);
}

bool Writer::writeXref(std::ofstream& outfile) { 
    uint64_t offset = 0;
    if (!getCurrentFilePosition(outfile, offset)) { return false; };
    xref.setXrefOffset(offset);
    return xref.writeXref(outfile);
}