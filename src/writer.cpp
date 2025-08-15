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
#include <cmath>
#include <vector>
#include <utility>

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

        int width = 256;
        int height = 256;
        int depth = 12;  // Match the dimensions array: {256, 256, 12, 5}
        int timePoints = 5;  // Match schema maxItems: 10
        int channels = 3;  // RGB

        dm.addMetaData({
            {"modality", "CT"},
            {"image_structure", {
                {"channels", channels},  // RGB image (3 channels)
                {"bit_depth", 8},
                {"origin", "top_left"},
                {"encoding", "jpeg2000-lossless"},
                {"memory_order", "row_major"},
                {"layout", "interleaved"},
                {"dimensions", {width, height, depth, timePoints}},    // 4D: width, height, depth, time
                {"dimension_names", {"x", "y", "z", "time"}}, 
            }},
            {"bodyPart", "CHEST"},
            {"institution", "Test Hospital"},
            {"acquisitionDate", "2024-01-01"},
            {"technician", "Dr. Smith"},
            {"patientName", "John Doe"},
            {"patientID", "12345"}
        });
        
        // Prepare frame data pairs for batch addition
        std::vector<std::pair<nlohmann::json, std::vector<uint8_t>>> frameDataPairs;
        
        // Create frames for each slice and time point
        for (int time = 0; time < timePoints; time++) {
            for (int slice = 0; slice < depth; slice++) {
                std::vector<uint8_t> sliceData(width * height * channels);  // RGB data
                
                // Fill with RGB pattern - create high contrast patterns that change with slice and time
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        size_t pixel_index = static_cast<size_t>(y * width + x);
                        
                        
                        // Z dimension (slice) affects brightness - creates a clear light-to-dark gradient
                        // First slice (z=0) is brightest, last slice (z=depth-1) is darkest
                        double zBrightness = 1.0 - (static_cast<double>(slice) / (depth - 1)) * 0.7;
                        zBrightness = std::max(0.3, std::min(1.0, zBrightness));
                        
                        // Time dimension affects color dominance - each time point gets a distinct color
                        uint8_t r, g, b;
                        
                        if (time == 0) {
                            // Time 0: Red dominant
                            r = static_cast<uint8_t>(255 * zBrightness);
                            g = static_cast<uint8_t>(80 * zBrightness);
                            b = static_cast<uint8_t>(80 * zBrightness);
                        } else if (time == 1) {
                            // Time 1: Green dominant
                            r = static_cast<uint8_t>(80 * zBrightness);
                            g = static_cast<uint8_t>(255 * zBrightness);
                            b = static_cast<uint8_t>(80 * zBrightness);
                        } else if (time == 2) {
                            // Time 2: Blue dominant
                            r = static_cast<uint8_t>(80 * zBrightness);
                            g = static_cast<uint8_t>(80 * zBrightness);
                            b = static_cast<uint8_t>(255 * zBrightness);
                        } else if (time == 3) {
                            // Time 3: Cyan dominant (Green + Blue)
                            r = static_cast<uint8_t>(80 * zBrightness);
                            g = static_cast<uint8_t>(255 * zBrightness);
                            b = static_cast<uint8_t>(255 * zBrightness);
                        } else {
                            // Time 4: Magenta dominant (Red + Blue)
                            r = static_cast<uint8_t>(255 * zBrightness);
                            g = static_cast<uint8_t>(80 * zBrightness);
                            b = static_cast<uint8_t>(255 * zBrightness);
                        }
                        
                        // Add subtle radial texture for visual interest
                        double textureFactor = 0.9 + 0.1 * sin(x * 0.1) * cos(y * 0.1);
                        r = static_cast<uint8_t>(r * textureFactor);
                        g = static_cast<uint8_t>(g * textureFactor);
                        b = static_cast<uint8_t>(b * textureFactor);
                        
                        // Interleaved RGB format: RGBRGBRGB...
                        sliceData[pixel_index * 3 + 0] = r;  // Red
                        sliceData[pixel_index * 3 + 1] = g;  // Green
                        sliceData[pixel_index * 3 + 2] = b;  // Blue
                    }
                }
                
                // Create frame metadata with 4D positioning
                nlohmann::json frameMetadata = {
                    {"position", {{"x", 0.0}, {"y", 0.0}, {"z", static_cast<double>(slice)}, {"t", static_cast<double>(time)}}},
                    {"orientation", {
                        {"row_cosine", {1.0, 0.0, 0.0}},
                        {"column_cosine", {0.0, 1.0, 0.0}}
                    }},
                    {"timestamp", "2024-01-01T12:00:00Z"},
                    {"frame_number", slice + time * depth},
                    {"time_point", time},
                    {"slice_number", slice}
                };
                
                // Add the pair to our collection
                frameDataPairs.push_back({frameMetadata, sliceData});
            }
        }
        
        // Add all frames at once using the new batch method
        dm.addData(frameDataPairs);
        
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

bool Writer::writeXref(std::ofstream& outfile) { 
    uint64_t offset = 0;
    if (!getCurrentFilePosition(outfile, offset)) { return false; };
    xref.setXrefOffset(offset);
    return xref.writeXref(outfile);
}