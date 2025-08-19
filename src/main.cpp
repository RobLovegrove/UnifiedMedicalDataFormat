#include <iostream>
#include <string>

#include "umdfFile.hpp"
#include "DataModule/dataModule.hpp"

#include "CLI11/CLI11.hpp"
#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"

using namespace std;
 

/* -------------------------- CONSTANTS -------------------------- */

/* -------------------------- DECLARATIONS -------------------------- */
void addWriteOptions(CLI::App* writeCmd, string& outputFile, bool& overwrite, bool& update);

/* -------------------------- MOCK DATA -------------------------- */


/* -------------------------- MAIN FUNCTION -------------------------- */

int main(int argc, char** argv) {

    string tabularUUID;

    UUID uuid;

    CLI::App app{"UMDF - Unified Medical Data Format Tool"};
    
    UMDFFile file; // Single UMDFFile instance for both read and write

    string inputFile;
    string outputFile;

    bool overwrite = false;
    bool update = false;

    // WRITE subcommand
    CLI::App* writeCmd = app.add_subcommand("write", "Write data to a UMDF file");
    addWriteOptions(writeCmd, outputFile, overwrite, update);

    // READ subcommand
    CLI::App* readCmd = app.add_subcommand("read", "Read data from a UMDF file");
    readCmd->add_option("-i,--input", inputFile, "Input UMDF file")->required();

    // Require that one subcommand is given
    app.require_subcommand();

    CLI11_PARSE(app, argc, argv);

    if (*writeCmd) {
        cout << "=== STEP 1: Writing new file with tabular data ===\n";
        cout << "Writing to file: " << outputFile << "\n";
        
        // Create modules data for the new file - ONLY tabular data initially
        std::vector<std::pair<std::string, ModuleData>> modulesWithSchemas;
        
        // Create a patient module (tabular data only)
        ModuleData patientModule;
        patientModule.metadata = nlohmann::json::array();
        patientModule.metadata.push_back({
            {"clinician", "Dr. Jane Doe"},
            {"encounter_time", "2025-07-28"}
        });
        
        // Add patient data
        nlohmann::json patientData = nlohmann::json::array();
        patientData.push_back({
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
        patientData.push_back({
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
        
        patientModule.data = patientData;
        modulesWithSchemas.push_back({"./schemas/patient/v1.0.json", patientModule});
        
        // Write the new file using UMDFFile (tabular data only)
        auto result = file.writeNewFile(outputFile, modulesWithSchemas);
        if (!result) {
            cerr << "Failed to write data\n";
            return 1;
        }

        cout << "Initial file written with tabular data. Module UUIDs:\n";
        for (const auto& uuid : result.value()) {
            cout << "  - " << uuid.toString() << endl;
        }

        cout << "\n=== STEP 2: Reading the file to verify tabular data ===\n";
        
        // Read the file to verify tabular data
        if (!file.openFile(outputFile)) {
            cerr << "Failed to open file for reading: " << outputFile << "\n";
            return 1;
        }
        
        auto fileInfo = file.getFileInfo();
        if (fileInfo.contains("success") && !fileInfo["success"]) {
            cerr << "Error reading file: " << fileInfo["error"] << "\n";
            return 1;
        }
        
        cout << "File opened successfully. Module count: " << fileInfo["module_count"] << "\n";
        
        // List all modules
        if (fileInfo.contains("modules")) {
            cout << "Modules in file:\n";
            for (const auto& module : fileInfo["modules"]) {
                if (module["type"] == "tabular") {
                    tabularUUID = module["uuid"];
                }
                cout << "  - " << module["type"] << " (UUID: " << module["uuid"] << ")\n";
            }
        }

        cout << "\n=== STEP 3: Adding image data using update API ===\n";
        
        // Create image module data
        ModuleData imageModule;
        imageModule.metadata = {
            {"acquisitionDate", "2024-01-01"},
            {"bodyPart", "CHEST"},
            {"image_structure", {
                {"bit_depth", 8},
                {"channels", 3},
                {"dimension_names", {"x", "y", "z", "time"}},
                {"dimensions", {256, 256, 12, 5}},
                {"encoding", "jpeg2000-lossless"},
                {"layout", "interleaved"},
                {"memory_order", "row_major"},
                {"origin", "top_left"}
            }},
            {"institution", "Test Hospital"},
            {"modality", "CT"},
            {"patientID", "12345"},
            {"patientName", "John Doe"},
            {"technician", "Dr. Smith"}
        };
        
        // Create sophisticated frame data with patterns
        std::vector<ModuleData> frameModules;
        int width = 256;
        int height = 256;
        int depth = 12;  // z dimension
        int timePoints = 5;  // time dimension
        
        // Create frames for each slice and time point
        for (int time = 0; time < timePoints; time++) {
            for (int slice = 0; slice < depth; slice++) {
                std::vector<uint8_t> sliceData(width * height * 3);  // RGB data
                
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
                            // Time 3: Yellow dominant (Red + Green)
                            r = static_cast<uint8_t>(255 * zBrightness);
                            g = static_cast<uint8_t>(255 * zBrightness);
                            b = static_cast<uint8_t>(80 * zBrightness);
                        } else {
                            // Time 4: Magenta dominant (Red + Blue)
                            r = static_cast<uint8_t>(255 * zBrightness);
                            g = static_cast<uint8_t>(80 * zBrightness);
                            b = static_cast<uint8_t>(255 * zBrightness);
                        }
                        
                        // Interleaved RGB format: RGBRGBRGB...
                        sliceData[pixel_index * 3 + 0] = r;  // Red
                        sliceData[pixel_index * 3 + 1] = g;  // Green
                        sliceData[pixel_index * 3 + 2] = b;  // Blue
                    }
                }
                
                // Create frame metadata with 4D positioning
                ModuleData frameModule;
                frameModule.metadata = {
                    {"position", {0.0, 0.0, static_cast<double>(slice), static_cast<double>(time)}},
                    {"orientation", {
                        {"row_cosine", {1.0, 0.0, 0.0}},
                        {"column_cosine", {0.0, 1.0, 0.0}}
                    }},
                    {"timestamp", "2024-01-01T12:00:00Z"},
                    {"frame_number", slice + time * depth},
                    {"time_point", time},
                    {"slice_number", slice}
                };
                
                frameModule.data = sliceData;
                frameModules.push_back(frameModule);
            }
        }
        
        imageModule.data = frameModules;
        
        // Add image data using the addModules API
        std::vector<std::pair<std::string, ModuleData>> imageModulesWithSchemas;
        imageModulesWithSchemas.push_back({"./schemas/image/v1.0.json", imageModule});
        
        auto addResult = file.addModules(outputFile, imageModulesWithSchemas);
        if (!addResult) {
            cerr << "Failed to add image data: " << addResult.error() << "\n";
            return 1;
        }
        
        cout << "Image data added successfully. New module UUIDs:\n";
        for (const auto& uuid : addResult.value()) {
            cout << "  - " << uuid.toString() << endl;
        }

        cout << "\n=== STEP 4: Rereading the file to show all data ===\n";
        
        // Close and reopen to get fresh file info
        file.closeFile();
        if (!file.openFile(outputFile)) {
            cerr << "Failed to reopen file for final reading: " << outputFile << "\n";
            return 1;
        }
        
        auto finalFileInfo = file.getFileInfo();
        if (finalFileInfo.contains("success") && !finalFileInfo["success"]) {
            cerr << "Error reading final file: " << finalFileInfo["error"] << "\n";
            return 1;
        }

        cout << "\n=== STEP 5: Update the Tabular Data ===\n";

        // Get the patient module data
        auto patientModuleData = file.getModuleData(tabularUUID);

        // Update the patient module data
        patientModuleData.value().metadata.push_back({
            {"clinician", "Dr. John Doe"},
            {"encounter_time", "2025-07-29"}
        });

        // Update the patient module data in the file
        std::vector<std::pair<std::string, ModuleData>> updates = {
            {tabularUUID, patientModuleData.value()}
        };

        if (!file.updateModules(updates)) {
            cerr << "Failed to update tabular data" << "\n";
            return 1;
        }
        
        cout << "Final file opened successfully. Module count: " << finalFileInfo["module_count"] << "\n";
        
        // List all modules
        if (finalFileInfo.contains("modules")) {
            cout << "All modules in file:\n";
            for (const auto& module : finalFileInfo["modules"]) {
                cout << "  - " << module["type"] << " (UUID: " << module["uuid"] << ")\n";
            }
        }

        cout << "\n=== Final data verification ===\n";
        
        // Load all modules to show final state
        for (const auto& module : finalFileInfo["modules"]) {
            cout << "Loading module: " << module["uuid"] << endl;
            auto moduleData = file.getModuleData(module["uuid"]);
            if (moduleData) {
                cout << "Module: " << module["type"] << " (UUID: " << module["uuid"] << ")" << endl;
                cout << "Metadata: " << moduleData.value().metadata.dump(2) << endl;
                
                // Handle the data variant based on type
                const auto& data = moduleData.value().data;
                if (std::holds_alternative<nlohmann::json>(data)) {
                    // Tabular data
                    cout << "Data (Tabular): " << std::get<nlohmann::json>(data).dump(2) << endl;
                } else if (std::holds_alternative<std::vector<uint8_t>>(data)) {
                    // Binary data (like image pixels)
                    const auto& binaryData = std::get<std::vector<uint8_t>>(data);
                    cout << "Data (Binary): " << binaryData.size() << " bytes" << endl;
                } else if (std::holds_alternative<std::vector<ModuleData>>(data)) {
                    // Nested modules (like image frames)
                    const auto& nestedModules = std::get<std::vector<ModuleData>>(data);
                    cout << "Data (Nested): " << nestedModules.size() << " sub-modules" << endl;
                    
                    // Show details of nested modules
                    for (size_t i = 0; i < nestedModules.size() && i < 3; i++) { // Limit to first 3 for readability
                        cout << "  Sub-module " << i << " metadata: " 
                             << nestedModules[i].metadata.dump(2) << endl;
                    }
                    if (nestedModules.size() > 3) {
                        cout << "  ... and " << (nestedModules.size() - 3) << " more sub-modules" << endl;
                    }
                }
                cout << endl;
            }
        }
        
        cout << "=== Full workflow demonstration complete! ===\n";
        cout << "File: " << outputFile << " now contains both tabular and image data.\n";
    }

    else if (*readCmd) {
        
        // Read file using new API
        cout << "Reading from file: " << inputFile << "\n";
        
        if (!file.openFile(inputFile)) {
            cerr << "Failed to open file: " << inputFile << "\n";
            return 1;
        }
        
        // Get file info
        auto fileInfo = file.getFileInfo();
        if (fileInfo.contains("success") && !fileInfo["success"]) {
            cerr << "Error reading file: " << fileInfo["error"] << "\n";
            return 1;
        }
        
        cout << "File opened successfully. Module count: " << fileInfo["module_count"] << "\n";
        
        // List all modules
        if (fileInfo.contains("modules")) {
            cout << "Modules in file:\n";
            for (const auto& module : fileInfo["modules"]) {
                cout << "  - " << module["type"] << " (UUID: " << module["uuid"] << ")\n";
            }
        }

        cout << endl;

        // Load all modules
        for (const auto& module : fileInfo["modules"]) {
            cout << "Loading module: " << module["uuid"] << endl;
            auto moduleData = file.getModuleData(module["uuid"]);
            if (moduleData) {
                cout << "Module: " << module["type"] << " (UUID: " << module["uuid"] << ")" << endl;
                cout << "Metadata: " << moduleData.value().metadata.dump(2) << endl;
                
                // Handle the data variant based on type
                const auto& data = moduleData.value().data;
                if (std::holds_alternative<nlohmann::json>(data)) {
                    // Tabular data
                    cout << "Data (Tabular): " << std::get<nlohmann::json>(data).dump(2) << endl;
                } else if (std::holds_alternative<std::vector<uint8_t>>(data)) {
                    // Binary data (like image pixels)
                    const auto& binaryData = std::get<std::vector<uint8_t>>(data);
                    cout << "Data (Binary): " << binaryData.size() << " bytes" << endl;
                } else if (std::holds_alternative<std::vector<ModuleData>>(data)) {
                    // Nested modules (like image frames)
                    const auto& nestedModules = std::get<std::vector<ModuleData>>(data);
                    cout << "Data (Nested): " << nestedModules.size() << " sub-modules" << endl;
                    
                    // Show details of nested modules
                    for (size_t i = 0; i < nestedModules.size() && i < 3; i++) { // Limit to first 3 for readability
                        cout << "  Sub-module " << i << " metadata: " 
                             << nestedModules[i].metadata.dump(2) << endl;
                    }
                    if (nestedModules.size() > 3) {
                        cout << "  ... and " << (nestedModules.size() - 3) << " more sub-modules" << endl;
                    }
                }
                cout << endl;
            }
        }
        
        cout << "File read complete" << endl;
    }

    return 0;
}



/* -------------------------- HELPER FUNCTIONS -------------------------- */

void addWriteOptions(CLI::App* writeCmd, string& outputFile, bool& overwrite, bool& update) {

    writeCmd->add_option("-o,--output", outputFile, "Output UMDF file")->required();

    writeCmd->add_flag("--overwrite", overwrite, "Overwrite existing UMDF file if it exists");
    writeCmd->add_flag("--update", update, "Update existing UMDF file instead of overwriting");

    writeCmd->callback([&]() {
        if (overwrite && update) {
            throw CLI::ValidationError("Cannot use both --overwrite and --update together.");
        }
    });
}