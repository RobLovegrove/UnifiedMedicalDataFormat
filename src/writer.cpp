#include "writer.hpp"
#include "Header/header.hpp"
#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"
#include "DataModule/Tabular/tabularData.hpp"
#include "DataModule/Image/imageData.hpp"
#include "DataModule/Image/FrameData.hpp"

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

    // CREATE TABULAR MODULE
    try {
        TabularData dm("./schemas/patient/v1.0.json", UUID());
        dm.addMetaData({
            {"clinician", "Dr. Jane Doe"},
            {"encounter_time", "2025-07-28"}
        });
        dm.addData({
            {"patient_id", "123e4567-e89b-12d3-a456-426614174000"},
            {"gender", "male"},
            {"birth_sex", "female"},
            {"birth_date", "1990-01-01"},
            {"name", {
                {"family", "Smith"},
                {"given", "Alice"}
            }},
            {"age", 29}
        });
        dm.addData({
            {"patient_id", "f49900f3-8dc7-47b9-b6f5-34939e4b42dc"},
            {"gender", "male"},
            {"birth_sex", "male"},
            {"birth_date", "1994-12-29"},
            {"name", {
                {"family", "Lovegrove"},
                {"given", "This is a long sentence that will be too long to store in a fixed length string"}
            }},
            {"age", 30}
        });
        dm.writeBinary(outfile, xref);

    }
    catch (runtime_error e) {
        cout << "Error: " << e.what() << endl;
    }

    cout << endl << endl;
    
    // CREATE IMAGE MODULE
    try {
        ImageData dm("./schemas/image/v1.0.json", UUID());

        dm.addMetaData({
            {"modality", "CT"},
            {"width", 16},
            {"height", 16},
            {"bit_depth", 8},
            {"encoding", "raw"},
            {"bodyPart", "CHEST"},
            {"institution", "Test Hospital"},
            {"acquisitionDate", "2024-01-01"},
            {"technician", "Dr. Smith"},
            {"patientName", "John Doe"},
            {"patientID", "12345"},
            {"dimensions", {16, 16, 8}},    // 3D: width, height, depth
            {"dimension_names", {"x", "y", "z"}}
        });
        
        // Get dimensions from metadata (hardcoded for now)
        int width = 16;
        int height = 16;
        int depth = 8;
        
        // Create frames for each slice
        for (int slice = 0; slice < depth; slice++) {
            auto frame = std::make_unique<FrameData>("./schemas/frame/v1.0.json", UUID());
            std::vector<uint8_t> sliceData(width * height);
            std::fill(sliceData.begin(), sliceData.end(), 128 + slice * 10); // different gray per slice
            frame->pixelData = sliceData;
            
            // Add frame metadata
            frame->addMetaData({
                {"position", {{"x", 0.0}, {"y", 0.0}, {"z", static_cast<double>(slice)}}},
                {"orientation", {
                    {"row_cosine", {1.0, 0.0, 0.0}},
                    {"column_cosine", {0.0, 1.0, 0.0}}
                }},
                {"timestamp", "2024-01-01T12:00:00Z"},
                {"frame_number", slice}
            });
            
            dm.addData(std::move(frame));
        }
        
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