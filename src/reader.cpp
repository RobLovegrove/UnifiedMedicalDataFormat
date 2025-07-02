#include "reader.hpp"
#include "header.hpp"
#include "utils.hpp"
#include "xref.hpp"

#include <fstream>
#include <cstdint>
#include <vector>
#include <iostream>

using namespace std;

bool Reader::readFile(const string& filename) { 
    
    ifstream inFile(filename, ios::binary);
    if (!inFile) return false;
    
    // Read header and confirm UMDF
    if (!header.readHeader(inFile)) { return false; } 

    // load Xref Table
    xrefTable = XRefTable::loadXrefTable(inFile);

    cout << "XRefTable successfully read" << endl;
    const XrefEntry* patientModule = xrefTable.findEntry(ModuleType::Patient);
    if (patientModule == nullptr) return false;

    inFile.seekg(patientModule->offset);

    std::vector<char> buffer(patientModule->size);
    inFile.read(buffer.data(), patientModule->size);

    string content(buffer.begin(), buffer.end());

    cout << "Moved to patient data offset: " << patientModule->offset << endl;
    cout << "Patient data of size: " << patientModule->size << endl;
    cout << "out putting patient data:\n" << content << endl;
    cout << endl;

    return true;
}
