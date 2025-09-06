#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "reader.hpp"
#include "writer.hpp"
#include "DataModule/dataModule.hpp"
#include "AuditTrail/auditTrail.hpp"

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

    Writer writer;
    Reader reader;

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
            {"encounter_date", "2025-07-28"}
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
        pair<string, ModuleData> patientPair = {"./schemas/patient/v1.0.json", patientModule};

        // Creating new UMDF file
        cout << "Creating new UMDF file: " << outputFile << endl;
        try {
        Result result = writer.createNewFile(outputFile, "rob", "password");
        if (!result.success) {
                cerr << "Failed to create new file: " << result.message << endl;
                return 1;
            }
        } catch (const std::exception& e) {
            cerr << "Failed to create new file: " << e.what() << endl;
            return 1;
        }

        UUID moduleId;

        auto encounterResult = writer.createNewEncounter();
        if (!encounterResult) {
            cerr << "Failed to create new encounter: " << encounterResult.error() << endl;
            return 1;
        }
        UUID encounterId = encounterResult.value();

        cout << "Adding tabular data to file" << endl;
        auto addResult = writer.addModuleToEncounter(encounterId, patientPair.first, patientPair.second);
        if (!addResult) {
            cerr << "Failed to add module: " << addResult.error() << endl;
            return 1;
        }
        moduleId = addResult.value();
        addResult = writer.addAnnotation(moduleId, patientPair.first, patientPair.second);
        if (!addResult) {
            cerr << "Failed to add annotation: " << addResult.error() << endl;
            return 1;
        }
        
        cout << "Initial file written with tabular data. Module UUIDs:\n";
        cout << "  - " << moduleId.toString() << endl;

        // Close the file
        cout << "Closing file" << endl;
        auto closeResult = writer.closeFile();
        if (!closeResult.success) {
            cerr << "Failed to close file: " << closeResult.message << endl;
            return 1;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));

        cout << "\n=== STEP 2: Reading the file to verify tabular data ===\n";
        
        // Read the file to verify tabular data
        auto readResult = reader.openFile(outputFile, "password");
        if (!readResult.success) {
            cerr << "Failed to open file for reading: " << outputFile << " " << readResult.message << endl;
            return 1;
        }
        
        auto fileInfo = reader.getFileInfo();
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

        reader.closeFile();

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
                //{"encoding", "raw"},
                //{"encoding", "png"},
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
        std::pair<std::string, ModuleData> imagePair = {"./schemas/image/v1.0.json", imageModule};

        cout << "Reopening file" << endl;
        auto reopenResult = writer.openFile(outputFile, "rob", "password");
        if (!reopenResult.success) {
            cerr << "Failed to reopen file: " << reopenResult.message << endl;
            return 1;
        }

        cout << "Adding image data to file" << endl;
        addResult = writer.addModuleToEncounter(encounterId, imagePair.first, imagePair.second);
        if (!addResult) {
            cerr << "Failed to add module: " << addResult.error() << endl;
            return 1;
        }
        UUID imageModuleId = addResult.value();

        
        // auto addResult = writer.addModules(outputFile, imageModulesWithSchemas);
        // if (!addResult) {
        //     cerr << "Failed to add image data: " << addResult.error() << "\n";
        //     return 1;
        // }
        
        cout << "Image data added successfully. New module UUID:\n";
        cout << "  - " << imageModuleId.toString() << endl;


        imagePair.second.metadata["modality"] = "CT2";

        // Add variant module
        cout << "Adding variant module" << endl;
        auto variantModuleResult = writer.addVariantModule(imageModuleId, imagePair.first, imagePair.second);
        if (!variantModuleResult) {
            cerr << "Failed to add variant module: " << variantModuleResult.error() << endl;
            return 1;
        }
        UUID variantModuleId = variantModuleResult.value();

        cout << "Variant module added successfully. New module UUID:\n";
        cout << "  - " << variantModuleId.toString() << endl;

        // Close the file
        cout << "Closing file" << endl;
        closeResult = writer.closeFile();
        if (!closeResult.success) {
            cerr << "Failed to close file: " << closeResult.message << endl;
            return 1;
        }

        cout << "\n=== STEP 4: Rereading the file to show all data ===\n";
        
        // Close and reopen to get fresh file info
        reopenResult = reader.openFile(outputFile, "password");
        if (!reopenResult.success) {
            cerr << "Failed to reopen file for final reading: " << outputFile << " " << reopenResult.message << endl;
            return 1;
        }
        
        auto finalFileInfo = reader.getFileInfo();
        if (finalFileInfo.contains("success") && !finalFileInfo["success"]) {
            cerr << "Error reading final file: " << finalFileInfo["error"] << "\n";
            return 1;
        }

        cout << "\n=== STEP 5: Update the Tabular Data ===\n";

        // Get the patient module data
        auto patientModuleData = reader.getModuleData(tabularUUID);
        if (!patientModuleData.has_value()) {
            cerr << "Failed to get patient module data" << " " << patientModuleData.error() << endl;
            return 1;
        }

        // Update the patient module data
        patientModuleData.value().metadata.push_back({
            {"clinician", "Dr. John Doe"},
            {"encounter_date", "2025-07-29"}
        });

        // Update the patient module data in the file
        reader.closeFile();

        auto result = writer.openFile(outputFile, "Bob", "password");
        if (!result.success) {
            cerr << "Failed to reopen file: " << result.message << endl;
            return 1;
        }

        cout << "Updating tabular data" << endl;
        auto updateResult = writer.updateModule(tabularUUID, patientModuleData.value());
        if (!updateResult.success) {
            cerr << "Failed to update module: " << updateResult.message << endl;
            return 1;
        }

        result = writer.closeFile();
        if (!result.success) {
            cerr << "Failed to close file: " << result.message << endl;
            return 1;
        }

        cout << "Closing file" << endl;
        
        cout << "Final file opened successfully. Module count: " << finalFileInfo["module_count"] << "\n";
        
        // List all modules
        if (finalFileInfo.contains("modules")) {
            cout << "All modules in file:\n";
            for (const auto& module : finalFileInfo["modules"]) {
                cout << "  - " << module["type"] << " (UUID: " << module["uuid"] << ")\n";
            }
        }
        cout << "\n=== Final data verification ===\n";
        reader.closeFile();

        result = reader.openFile(outputFile, "password");
        if (!result.success) {
            cerr << "Failed to reopen file: " << result.message << endl;
            return 1;
        }

        finalFileInfo = reader.getFileInfo();


        cout << "Encounter path: " << endl;
        reader.printEncounterPath(encounterId);
        cout << "finalFileInfo: " << finalFileInfo.dump(2) << endl;


        auto auditTrailResult = reader.getAuditTrail(UUID::fromString(tabularUUID));
        if (!auditTrailResult.has_value()) {
            cerr << "Failed to get audit trail: " << auditTrailResult.error() << endl;
            return 1;
        }
        std::vector<ModuleTrail> auditTrail = auditTrailResult.value(); 
        cout << endl <<"Audit trail: " << auditTrail.size() << " entries" << endl;
        for (const auto& trail : auditTrail) {
            cout << "Audit trail entry: " 
            << "Module offset: " << trail.moduleOffset << "\n" 
            << "Is current: " << trail.isCurrent << "\n" 
            << "Created at: " << trail.createdAt.toString() << "\n" 
            << "Modified at: " << trail.modifiedAt.toString() << "\n" 
            << "Created by: " << trail.createdBy << "\n" 
            << "Modified by: " << trail.modifiedBy << endl
            << "Module size: " << trail.moduleSize << endl << endl;
        }

        auto moduleData = reader.getAuditData(auditTrail[auditTrail.size() - 1]);
        if (!moduleData.has_value()) {
            cerr << "Failed to get module data: " << moduleData.error() << endl;
            return 1;
        }
        cout << "\n\nOLD MODULE DATA: \n\n" << moduleData.value().metadata.dump(2) << endl;
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

        // Load all modules to show final state
        for (const auto& module : finalFileInfo["modules"]) {

            cout << "Module: " << module["type"] << " (UUID: " << module["uuid"] << ")" << endl;

            auto moduleData = reader.getModuleData(module["uuid"]);
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

        // Remove the file
        std::filesystem::remove(outputFile);
    }

    else if (*readCmd) {
        
        // Read file using new API
        cout << "Reading from file: " << inputFile << "\n";
        
        auto result = reader.openFile(inputFile, "password");
        if (!result.success) {
            cerr << "Failed to open file: " << inputFile << " " << result.message << endl;
            return 1;
        }
        
        // Get file info
        auto fileInfo = reader.getFileInfo();
        if (fileInfo.contains("success") && !fileInfo["success"]) {
            cerr << "Error reading file: " << fileInfo["error"] << endl;
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

            auto moduleData = reader.getModuleData(module["uuid"]);
            if (moduleData.has_value()) {
                cout << "Module: " << module["type"] << " (UUID: " << module["uuid"] << ")" << endl;
                cout << "Metadata: " << moduleData.value().metadata.dump(2) << endl;
                
                // Handle the data variant based on type
                const auto& data = moduleData.value().data;
                if (std::holds_alternative<nlohmann::json>(data)) {
                    // Tabular data
                    cout << "Data (Tabular): " << std::get<nlohmann::json>(data).dump(2) << endl;
                } else if (std::holds_alternative<std::vector<uint8_t>>(data)) {
                    // Binary data 
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
                        
                        // Print pixel data for each frame
                        const auto& frameData = nestedModules[i].data;
                        if (std::holds_alternative<std::vector<uint8_t>>(frameData)) {
                            const auto& pixelData = std::get<std::vector<uint8_t>>(frameData);
                            cout << "  Frame " << i << " pixel data: " << pixelData.size() << " pixels" << endl;
                            
                            // Print first 10,000 pixel values for preview
                            cout << "  First 100,000 pixel values: ";
                            for (size_t j = 0; j < min(pixelData.size(), (size_t)100000); j++) {
                                cout << (int)pixelData[j] << " ";
                            }
                            if (pixelData.size() > 100000) {
                                cout << "... (and " << (pixelData.size() - 100000) << " more)";
                            }
                            cout << endl;
                            
                            // Print pixel statistics
                            if (!pixelData.empty()) {
                                uint8_t minVal = *min_element(pixelData.begin(), pixelData.end());
                                uint8_t maxVal = *max_element(pixelData.begin(), pixelData.end());
                                cout << "  Pixel range: [" << (int)minVal << "-" << (int)maxVal << "]" << endl;
                            }
                        }
                    }
                    if (nestedModules.size() > 3) {
                        cout << "  ... and " << (nestedModules.size() - 3) << " more sub-modules" << endl;
                    }
                }
                cout << endl;
            }
            else {
                cout << "Error loading module: " << module["uuid"] << " " << moduleData.error() << endl;
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