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
        DataModule dm("./schemas/patient/v1.0.json");
        nlohmann::json schema = dm.getSchema();
        dm.addRow({
            {"patient_id", "123e4567-e89b-12d3-a456-426614174000"},
            {"gender", "male"},
            {"birth_sex", "female"},
            {"birth_date", "1990-01-01"},
            {"name", "Alice Smith"}
        });
        dm.writeBinary(outfile);

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