#include "reader.hpp"
#include "Header/header.hpp"
#include "Utility/utils.hpp"
#include "Xref/xref.hpp"
#include "DataModule/dataHeader.hpp"
#include "DataModule/dataModule.hpp"


#include <fstream>
#include <cstdint>
#include <vector>
#include <iostream>
#include <string>
#include <sstream> 

using namespace std;

bool Reader::readFile(const string& filename) { 
    
    ifstream inFile(filename, ios::binary);
    if (!inFile) return false;
    
    // Read header and confirm UMDF
    if (!header.readPrimaryHeader(inFile)) { return false; } 

    // load Xref Table
    xrefTable = XRefTable::loadXrefTable(inFile);

    cout << "XRefTable successfully read" << endl;
    cout << xrefTable << endl;

    constexpr size_t MAX_IN_MEMORY_MODULE_SIZE = 1 << 20; // 1 << 20 is bitwise 1 * 2^20 = 1,048,576 (1 megabyte) 

    // Iterate through XrefTable reading data
    for (const XrefEntry& entry : xrefTable.getEntries()) {
        if (entry.type == static_cast<uint8_t>(ModuleType::Tabular)) {
            if (entry.size <= MAX_IN_MEMORY_MODULE_SIZE) {
                vector<char> buffer(entry.size);
                inFile.seekg(entry.offset);
                inFile.read(buffer.data(), entry.size);
                istringstream stream(string(buffer.begin(), buffer.end()));

                unique_ptr<DataModule> dm = DataModule::fromStream(stream);
                dm->printRows(cout);

            }
            else {
                //unique_ptr<DataModule> dm = DataModule::fromFile(inFile, entry.offset);

            }
        }
    }
    return true;
}
