#include "writer.hpp"
#include "Header/header.hpp"
#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"
#include "DataModule/dataModule.hpp"

#include <nlohmann/json.hpp> 

#include <fstream>
#include <cstdint>
#include <iostream>
#include <string>

using namespace std;

bool Writer::writeNewFile(const string& filename) {

    // OPEN FILE
    ofstream outfile(filename, ios::binary);
    if (!outfile) return false;

    // CREATE HEADER
    if (!header.writePrimaryHeader(outfile, xref)) return false;

    // CREATE DATA MODULE
    try {
        DataModule dm("./schemas/patient/v1.0.json", UUID());
        dm.addRow({
            {"patient_id", "123e4567-e89b-12d3-a456-426614174000"},
            {"gender", "male"},
            {"birth_sex", "female"},
            {"birth_date", "1990-01-01"},
            {"name", {
                {"family", "Smith"},
                {"given", "Alice"}
            }}
        });
        dm.addRow({
            {"patient_id", "f49900f3-8dc7-47b9-b6f5-34939e4b42dc"},
            {"gender", "male"},
            {"birth_sex", "male"},
            {"birth_date", "1994-12-29"},
            {"name", {
                {"family", "Lovegrove"},
                {"given", "Sir Robert of the Squire, earl of the Thames, Mayor of sunbury, Lord of the south"}
            }}
        });
        dm.writeBinary(outfile, xref);

    }
    catch (runtime_error e) {
        cout << "Error: " << e.what() << endl;
    }

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