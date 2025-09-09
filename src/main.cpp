#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include "reader.hpp"
#include "writer.hpp"
#include "DataModule/dataModule.hpp"
#include "AuditTrail/auditTrail.hpp"

#include "CLI11/CLI11.hpp"
#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"
#include "mockDataLoader.hpp"

using namespace std;

/* -------------------------- DECLARATIONS -------------------------- */
struct CLISubcommands {
    CLI::App* demoCmd;
    CLI::App* writeCmd;
    CLI::App* readCmd;
    CLI::App* createCmd;
    CLI::App* addCmd;
    CLI::App* updateCmd;
    CLI::App* addVariantCmd;
    CLI::App* addAnnotationCmd;
};

CLISubcommands setUpCLI(CLI::App* app, string& inputFile, string& outputFile, string& password, string& author, string& encounterId);
void addDemoOptions(CLI::App* demoCmd, string& outputFile);
void displayModuleTree(Reader& reader, const nlohmann::json& moduleTree, int indentLevel);
void displayModuleData(ModuleData& moduleData, const string& moduleType = "unknown", const string& moduleUuid = "unknown");
void displayFileData(Reader& reader, const nlohmann::json& fileInfo, bool showSummary = true);
void displayOperationHeader(const string& operation, const string& inputFile, const string& outputFile, const string& encounterId, const string& author, const string& password);
void displayEncryptionStatus(const string& password);
bool loadMockData(const string& inputFile, string& schemaPath, ModuleData& moduleData);
bool openOrCreateFile(Writer& writer, const string& operation, string& outputFile, const string& author, const string& password);
bool closeFile(Writer& writer);

/* -------------------------- MAIN FUNCTION -------------------------- */

int main(int argc, char** argv) {
    // Initialize variables
    string firstTabularUUID;
    string tabularUUID;
    UUID uuid;

    CLI::App app{"UMDF - Unified Medical Data Format Tool"};

    Writer writer;
    Reader reader;

    string inputFile;
    string outputFile;
    string author = "User (Default)";
    string password = "";
    string encounterId = "";

    // Set up all CLI commands
    CLISubcommands subcommands = setUpCLI(&app, inputFile, outputFile, password, author, encounterId);

    CLI11_PARSE(app, argc, argv);

    if (*subcommands.demoCmd) {
        cout << "\n" << string(80, '=') << "\n";
        cout << "                        UMDF SYSTEM DEMONSTRATION\n";
        cout << "             Unified Medical Data Format - Complete Workflow\n";
        cout << string(80, '=') << "\n\n";
        
        cout << "This demonstration will show the complete UMDF workflow:\n";
        cout << "1. Creating a new UMDF file with patient data\n";
        cout << "2. Reading and verifying the data\n";
        cout << "3. Adding medical imaging data\n";
        cout << "4. Creating module variants\n";
        cout << "5. Updating existing data and displaying audit trail\n";
        cout << "6. Final verification and cleanup\n\n";
        
        cout << "Output file: " << outputFile << "\n\n";
        
        try {
            // Load patient mock data
            cout << "STEP 1: LOADING MOCK DATA\n";
            cout << string(40, '-') << "\n";
            cout << "Loading patient demographic data from mock_data/patient_data.json...\n";
            auto patientPair = MockDataLoader::loadMockData("mock_data/patient_data.json");
            cout << "Patient data loaded successfully\n\n";

            // Creating new UMDF file
            cout << "STEP 2: CREATING NEW UMDF FILE\n";
            cout << string(40, '-') << "\n";
            cout << "Creating new UMDF file: " << outputFile << "\n";
             cout << "Author: " << author << "\n";
             if (password != "") {
                 cout << "Encryption: AES-256-GCM enabled\n";
                 cout << "Password: " << password << "\n";
             }
             else {
                 cout << "Encryption: NONE\n";
             }
            
            try {
            Result result = writer.createNewFile(outputFile, author, password);
        if (!result.success) {
                cerr << "Failed to create new file: " << result.message << endl;
                return 1;
            }
        } catch (const std::exception& e) {
            cerr << "Failed to create new file: " << e.what() << endl;
            return 1;
        }
            cout << "UMDF file created successfully\n\n";

        UUID moduleId;

            cout << "Creating new medical encounter...\n";
        auto encounterResult = writer.createNewEncounter();
        if (!encounterResult) {
            cerr << "Failed to create new encounter: " << encounterResult.error() << endl;
            return 1;
        }
        UUID encounterId = encounterResult.value();
            cout << "Medical encounter created (ID: " << encounterId.toString() << ")\n\n";

            cout << "Adding patient demographic data to encounter...\n";
        auto addResult = writer.addModuleToEncounter(encounterId, patientPair.first, patientPair.second);
        if (!addResult) {
                cerr << "Failed to add patient module: " << addResult.error() << endl;
            return 1;
        }
        moduleId = addResult.value();
        firstTabularUUID = moduleId.toString();
            
            cout << "\n\nAdding clinical annotation to patient data...\n";
        addResult = writer.addAnnotation(moduleId, patientPair.first, patientPair.second);
        if (!addResult) {
            cerr << "Failed to add annotation: " << addResult.error() << endl;
            return 1;
        }
        
            cout << "\nPatient data module added successfully\n";
            cout << "  Module UUID: " << moduleId.toString() << "\n";
            cout << "  Schema: " << patientPair.first << "\n\n";

        // Close the file
            cout << "Closing file and finalizing...\n";
        auto closeResult = writer.closeFile();
        if (!closeResult.success) {
            cerr << "Failed to close file: " << closeResult.message << endl;
            return 1;
        }
            cout << "File closed and saved successfully\n\n";

            cout << "STEP 3: DATA VERIFICATION\n";
            cout << string(40, '-') << "\n";
            cout << "Reading the file to verify patient data was stored correctly...\n";
        
        // Read the file to verify tabular data
            auto readResult = reader.openFile(outputFile, password);
        if (!readResult.success) {
            cerr << "Failed to open file for reading: " << outputFile << " " << readResult.message << endl;
            return 1;
        }
        
        auto fileInfo = reader.getFileInfo();
        if (fileInfo.contains("success") && !fileInfo["success"]) {
            cerr << "Error reading file: " << fileInfo["error"] << "\n";
            return 1;
        }
        
            cout << "File opened successfully\n";
            cout << "  Total modules: " << fileInfo["module_count"] << "\n";
        
        // List all modules
        if (fileInfo.contains("modules")) {
                cout << "  Modules found:\n";
            for (const auto& module : fileInfo["modules"]) {
                if (module["type"] == "tabular") {
                    tabularUUID = module["uuid"];
                    }
                    cout << "    - " << module["type"] << " data (UUID: " << module["uuid"] << ")\n";
                }
            }

            cout << "\nModule graph:" << endl;
            cout << fileInfo["module_graph"].dump(2) << endl;

            reader.closeFile();
            cout << "\nData verification complete\n\n";

            cout << "STEP 4: ADDING MEDICAL IMAGING DATA\n";
            cout << string(40, '-') << "\n";
            
            // Load image mock data
            cout << "Loading CT scan imaging data from mock_data/ct_image_data.json...\n";
            auto imagePair = MockDataLoader::loadMockData("mock_data/ct_image_data.json");
            cout << "\nCT imaging data loaded successfully\n\n";

            cout << "Reopening file for additional data...\n";
            auto reopenResult = writer.openFile(outputFile, author, password);
        if (!reopenResult.success) {
            cerr << "Failed to reopen file: " << reopenResult.message << endl;
            return 1;
        }
            cout << "File reopened successfully\n\n";

            cout << "Adding CT scan data to the same medical encounter...\n";
            cout << "  - Modality: CT (Computed Tomography)\n";
            cout << "  - Dimensions: 256x256x12x5 (x,y,z,time)\n";
            cout << "  - Encoding: PNG compression\n";
            cout << "  - Frames: 60 total (12 slices × 5 time points)\n\n";
            
        addResult = writer.addModuleToEncounter(encounterId, imagePair.first, imagePair.second);
        if (!addResult) {
                cerr << "Failed to add image module: " << addResult.error() << endl;
            return 1;
        }
        UUID imageModuleId = addResult.value();

            cout << "Imaging data added successfully\n";
            cout << "  Module UUID: " << imageModuleId.toString() << "\n";
            cout << "  Schema: " << imagePair.first << "\n\n";


            cout << "STEP 5: CREATING MODULE VARIANT\n";
            cout << string(40, '-') << "\n";
            cout << "Creating a variant of the CT scan with modified parameters...\n";
            cout << "  - Original modality: CT\n";
            cout << "  - Variant modality: MRI (modified processing)\n";

            imagePair.second.metadata["modality"] = "MRI";

        // Add variant module
        auto variantModuleResult = writer.addVariantModule(imageModuleId, imagePair.first, imagePair.second);
        if (!variantModuleResult) {
            cerr << "Failed to add variant module: " << variantModuleResult.error() << endl;
            return 1;
        }
        UUID variantModuleId = variantModuleResult.value();

            cout << "\nVariant module created successfully\n";
            cout << "  Variant UUID: " << variantModuleId.toString() << "\n";
            cout << "  Parent UUID: " << imageModuleId.toString() << "\n\n";

        // Close the file
            cout << "Closing file and finalizing imaging data...\n";
        closeResult = writer.closeFile();
        if (!closeResult.success) {
            cerr << "Failed to close file: " << closeResult.message << endl;
            return 1;
        }
            cout << "File closed and imaging data saved\n\n";

            cout << "STEP 6: DATA VERIFICATION\n";
            cout << string(40, '-') << "\n";
            cout << "Reading the complete file to verify all data types...\n";
        
        // Close and reopen to get fresh file info
            reopenResult = reader.openFile(outputFile, password);
        if (!reopenResult.success) {
            cerr << "Failed to reopen file for final reading: " << outputFile << " " << reopenResult.message << endl;
            return 1;
        }
        
        auto finalFileInfo = reader.getFileInfo();
        if (finalFileInfo.contains("success") && !finalFileInfo["success"]) {
            cerr << "Error reading final file: " << finalFileInfo["error"] << "\n";
            return 1;
        }

            cout << "File reopened successfully\n";
            cout << "  Total modules: " << finalFileInfo["module_count"] << "\n";
            cout << "  Data types: Patient demographics + CT imaging + Variants\n\n";

            cout << "Module graph:" << endl;
            cout << finalFileInfo["module_graph"].dump(2) << endl;

            cout << "\n\nSTEP 7: UPDATING DATA\n";
            cout << string(40, '-') << "\n";
            cout << "INTRODUCING ARTIFICIAL TIME DELAY\n";
            cout << "This 2-second delay is added to demonstrate the audit trail\n";
            cout << "showing different timestamps for module creation vs. modification.\n";
            
            cout << "Waiting 3 seconds.";
            cout.flush();
            for (int i = 0; i < 3; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                cout << ".";
                cout.flush();
            }
            cout << " Done!\n\n";

        // Get the patient module data
            cout << "Retrieving current patient data for update...\n";
        auto patientModuleData = reader.getModuleData(tabularUUID);
        if (!patientModuleData.has_value()) {
                cerr << "Failed to get patient module data: " << patientModuleData.error() << endl;
            return 1;
        }
            cout << "\nPatient data retrieved successfully\n\n";

        // Update the patient module data
            cout << "Adding new clinical annotation to patient data...\n";
            
        patientModuleData.value().metadata.push_back({
            {"clinician", "Dr. John Doe"},
            {"encounter_date", "2025-07-29"}
        });

        // Update the patient module data in the file
        reader.closeFile();

            string newAuthor = "Rob";
            cout << "\nOpening file for update with a new author: " << newAuthor << ")...\n";
            auto result = writer.openFile(outputFile, newAuthor, password);
        if (!result.success) {
            cerr << "Failed to reopen file: " << result.message << endl;
            return 1;
        }
            cout << "File opened for update by user '" << newAuthor << "'\n\n";

            cout << "Updating patient module with new clinical data...\n";
        auto updateResult = writer.updateModule(firstTabularUUID, patientModuleData.value());
        if (!updateResult.success) {
            cerr << "Failed to update module: " << updateResult.message << endl;
            return 1;
        }
            cout << "Patient data updated successfully\n\n";

        result = writer.closeFile();
        if (!result.success) {
            cerr << "Failed to close file: " << result.message << endl;
            return 1;
        }
            cout << "File closed after update\n\n";

            cout << "STEP 8: FINAL VERIFICATION AND AUDIT TRAIL\n";
            cout << string(40, '-') << "\n";
            cout << "Reading final file state and displaying audit trail...\n\n";

            cout << "Reopening file for final verification...\n";
            result = reader.openFile(outputFile, password);
        if (!result.success) {
            cerr << "Failed to reopen file: " << result.message << endl;
            return 1;
        }

            cout << "File reopened successfully\n\n";
        finalFileInfo = reader.getFileInfo();

            // Display file data 
            displayFileData(reader, finalFileInfo, true);

            cout << "\nAUDIT TRAIL DEMONSTRATION\n";
            cout << "========================\n";
            cout << "The audit trail shows the complete history of the patient tabular module:\n\n";

        auto auditTrailResult = reader.getAuditTrail(UUID::fromString(firstTabularUUID));
        if (!auditTrailResult.has_value()) {
            cerr << "Failed to get audit trail: " << auditTrailResult.error() << endl;
            return 1;
        }
        std::vector<ModuleTrail> auditTrail = auditTrailResult.value(); 
            
            cout << "Audit trail entries: " << auditTrail.size() << "\n";
            cout << "This shows the complete modification history:\n\n";
            
            for (size_t i = 0; i < auditTrail.size(); i++) {
                const auto& trail = auditTrail[i];
                cout << "Entry " << (i + 1) << ":\n";
                cout << "  Status: " << (trail.isCurrent ? "CURRENT VERSION" : "PREVIOUS VERSION") << "\n";
                cout << "  Created: " << trail.createdAt.toString() << " by " << trail.createdBy << "\n";
                cout << "  Modified: " << trail.modifiedAt.toString() << " by " << trail.modifiedBy << "\n";
                cout << "  Size: " << trail.moduleSize << " bytes\n";
                cout << "  Offset: " << trail.moduleOffset << "\n\n";
                auto auditData = reader.getAuditData(trail);
                if (auditData) {
                    displayModuleData(auditData.value(), module_type_to_string(trail.moduleType), trail.moduleID.toString());
                }
            }

            cout << "DEMONSTRATION COMPLETE\n";
            cout << "=====================\n";
            cout << "The UMDF system has successfully demonstrated:\n";
            cout << "Secure file creation with encryption\n";
            cout << "Patient demographic data storage\n";
            cout << "Medical imaging data with compression (with simple mock data)\n";
            cout << "Module Graph capabilities\n";
            cout << "Complete audit trail with timestamps\n";
            cout << "Multi-user access (" << author << " → " << newAuthor << ")\n";
            cout << "FILE CLEANUP\n";
            cout << "============\n";

            // Remove the file only if using default filename
            if (outputFile == "demo.umdf") {
                std::filesystem::remove(outputFile);
                cout << "Demo file cleaned up (temporary file removed)\n";
            } else {
                cout << "File preserved: " << outputFile << "\n";
                cout << "  Your custom file has been saved and can be used for further testing.\n\n";
            }
            
            cout << "\n";
            cout << string(80, '=') << "\n";
            cout << "\n                          UMDF DEMONSTRATION COMPLETE\n\n";
            cout << string(80, '=') << "\n\n";
            
        } catch (const std::exception& e) {
            cerr << "Demo error: " << e.what() << endl;
            return 1;
        }
    }

    else if (*subcommands.writeCmd) {
        // Check for write subcommands
        if (*subcommands.createCmd) {
            displayOperationHeader("Creating new UMDF file", inputFile, outputFile, "", author, password);
            
            try {
                // Load mock data from file
                string schemaPath;
                ModuleData moduleData;
                if (!loadMockData(inputFile, schemaPath, moduleData)) {
                    return 1;
                }
                
                // Create new file
                if (!openOrCreateFile(writer, "create", outputFile, author, password)) {
                    return 1;
                }
                
                // Create new encounter
                auto encounterResult = writer.createNewEncounter();
                if (!encounterResult) {
                    cerr << "Failed to create new encounter: " << encounterResult.error() << endl;
                    return 1;
                }
                UUID encounterUUID = encounterResult.value();
                cout << "Created new encounter: " << encounterUUID.toString() << endl;
                
                // Add module to encounter
                cout << "Adding module to file" << endl;
                auto addResult = writer.addModuleToEncounter(encounterUUID, schemaPath, moduleData);
                if (!addResult) {
                    cerr << "Failed to add module: " << addResult.error() << endl;
                    return 1;
                }
                
                UUID moduleId = addResult.value();
                cout << "Module added successfully. UUID: " << moduleId.toString() << endl;
                
                // Close the file
                if (!closeFile(writer)) {
                    return 1;
                }
                
                cout << "UMDF file created successfully: " << outputFile << endl;
                
            } catch (const std::exception& e) {
                cerr << "Error: " << e.what() << endl;
                return 1;
            }
        }
        else if (*subcommands.addCmd) {
            displayOperationHeader("Adding module to existing UMDF file", inputFile, outputFile, encounterId, author, password);
            
            try {
                // Load mock data from file
                string schemaPath;
                ModuleData moduleData;
                if (!loadMockData(inputFile, schemaPath, moduleData)) {
                    return 1;
                }
                
                // Add to existing file
                if (!openOrCreateFile(writer, "add", outputFile, author, password)) {
                    return 1;
                }
                
                UUID encounterUUID;
                if (!encounterId.empty()) {
                    encounterUUID = UUID::fromString(encounterId);
                    cout << "Using existing encounter: " << encounterUUID.toString() << endl;
                } else {
                    auto encounterResult = writer.createNewEncounter();
                    if (!encounterResult) {
                        cerr << "Failed to create new encounter: " << encounterResult.error() << endl;
                        return 1;
                    }
                    encounterUUID = encounterResult.value();
                    cout << "Created new encounter: " << encounterUUID.toString() << endl;
                }
                
                // Add module to encounter
                cout << "Adding module to encounter" << endl;
                auto addResult = writer.addModuleToEncounter(encounterUUID, schemaPath, moduleData);
                if (!addResult) {
                    cerr << "Failed to add module: " << addResult.error() << endl;
                    return 1;
                }
                
                UUID moduleId = addResult.value();
                cout << "Module added successfully. UUID: " << moduleId.toString() << endl;
                
                // Close the file
                if (!closeFile(writer)) {
                    return 1;
                }
                
                cout << "Module added successfully to: " << outputFile << endl;
                
            } catch (const std::exception& e) {
                cerr << "Error: " << e.what() << endl;
                return 1;
            }
        }
        else if (*subcommands.updateCmd) {
            displayOperationHeader("Updating module in UMDF file", inputFile, outputFile, encounterId, author, password);
            
            try {
                // Load mock data from file
                string schemaPath;
                ModuleData moduleData;
                if (!loadMockData(inputFile, schemaPath, moduleData)) {
                    return 1;
                }
                
                // Open existing file for update
                if (!openOrCreateFile(writer, "update", outputFile, author, password)) {
                    return 1;
                }
                
                // Convert module ID string to UUID
                UUID moduleUUID = UUID::fromString(encounterId);
                cout << "Updating module: " << moduleUUID.toString() << endl;
                
                // Update the module
                auto updateResult = writer.updateModule(encounterId, moduleData);
                if (!updateResult.success) {
                    cerr << "Failed to update module: " << updateResult.message << endl;
                    return 1;
                }
                
                cout << "Module updated successfully" << endl;
                
                // Close the file
                if (!closeFile(writer)) {
                    return 1;
                }
                
                cout << "UMDF file updated successfully: " << outputFile << endl;
                
            } catch (const std::exception& e) {
                cerr << "Error: " << e.what() << endl;
                return 1;
            }
        }
        else if (*subcommands.addVariantCmd) {
            displayOperationHeader("Adding variant module to parent", inputFile, outputFile, encounterId, author, password);
            
            try {
                // Load mock data from file
                string schemaPath;
                ModuleData moduleData;
                if (!loadMockData(inputFile, schemaPath, moduleData)) {
                    return 1;
                }
                
                // Open existing file for adding variant
                if (!openOrCreateFile(writer, "addVariant", outputFile, author, password)) {
                    return 1;
                }
                
                // Convert parent module ID string to UUID
                UUID parentModuleUUID = UUID::fromString(encounterId);
                cout << "Adding variant to parent module: " << parentModuleUUID.toString() << endl;
                
                // Add variant module
                auto variantResult = writer.addVariantModule(parentModuleUUID, schemaPath, moduleData);
                if (!variantResult) {
                    cerr << "Failed to add variant module: " << variantResult.error() << endl;
                    return 1;
                }
                
                UUID variantModuleId = variantResult.value();
                cout << "Variant module added successfully. UUID: " << variantModuleId.toString() << endl;
                
                // Close the file
                if (!closeFile(writer)) {
                    return 1;
                }
                
                cout << "Variant module added successfully to: " << outputFile << endl;
                
            } catch (const std::exception& e) {
                cerr << "Error: " << e.what() << endl;
                return 1;
            }
        }
        else if (*subcommands.addAnnotationCmd) {
            displayOperationHeader("Adding annotation module to parent", inputFile, outputFile, encounterId, author, password);
            
            try {
                // Load mock data from file
                string schemaPath;
                ModuleData moduleData;
                if (!loadMockData(inputFile, schemaPath, moduleData)) {
                    return 1;
                }
                
                // Open existing file for adding annotation
                if (!openOrCreateFile(writer, "addAnnotation", outputFile, author, password)) {
                    return 1;
                }
                
                // Convert parent module ID string to UUID
                UUID parentModuleUUID = UUID::fromString(encounterId);
                cout << "Adding annotation to parent module: " << parentModuleUUID.toString() << endl;
                
                // Add annotation module
                auto annotationResult = writer.addAnnotation(parentModuleUUID, schemaPath, moduleData);
                if (!annotationResult) {
                    cerr << "Failed to add annotation module: " << annotationResult.error() << endl;
                    return 1;
                }
                
                UUID annotationModuleId = annotationResult.value();
                cout << "Annotation module added successfully. UUID: " << annotationModuleId.toString() << endl;
                
                // Close the file
                if (!closeFile(writer)) {
                    return 1;
                }
                
                cout << "Annotation module added successfully to: " << outputFile << endl;
                
            } catch (const std::exception& e) {
                cerr << "Error: " << e.what() << endl;
                return 1;
            }
        }
        else {
            cerr << "Error: No write subcommand specified. Use 'create', 'add', 'update', 'addVariant', or 'addAnnotation'" << endl;
            return 1;
        }
    }

    else if (*subcommands.readCmd) {
        
        // Read file using new API
        cout << "Reading from file: " << inputFile << "\n";
        
        auto result = reader.openFile(inputFile, password);
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
        
        // Display file data using the reusable function
        displayFileData(reader, fileInfo, false);
        
        cout << "File read complete" << endl;
    }


    return 0;
}



/* -------------------------- HELPER FUNCTIONS -------------------------- */

CLISubcommands setUpCLI(CLI::App* app, string& inputFile, string& outputFile, string& password, string& author, string& encounterId) {
    // DEMO subcommand
    CLI::App* demoCmd = app->add_subcommand("demo", "Run a comprehensive demonstration of UMDF capabilities with sample data");
    addDemoOptions(demoCmd, outputFile);
    demoCmd->add_option("-p,--password", password, "Password for encrypted UMDF file (optional)");
    demoCmd->add_option("-a,--author", author, "Author of the UMDF file (default: 'User (Default)')");

    // WRITE subcommand with sub-subcommands
    CLI::App* writeCmd = app->add_subcommand("write", "Write data to a UMDF file from mock data");

    // Write subcommands - all write operations as subcommands of write
    CLI::App* createCmd = writeCmd->add_subcommand("create", "Create a new UMDF file");
    createCmd->add_option("-i,--input", inputFile, "Input mock data file")->required();
    createCmd->add_option("-o,--output", outputFile, "Output UMDF file")->required();
    createCmd->add_option("-e,--encounter-id", encounterId, "Encounter ID to add module to (optional, creates new encounter if not provided)");
    createCmd->add_option("-p,--password", password, "Password for encrypted UMDF file (optional)");
    createCmd->add_option("-a,--author", author, "Author of the UMDF file (default: 'User (Default)')");

    CLI::App* addCmd = writeCmd->add_subcommand("add", "Add module to existing UMDF file");
    addCmd->add_option("-i,--input", inputFile, "Input mock data file")->required();
    addCmd->add_option("-o,--output", outputFile, "UMDF file to add to")->required();
    addCmd->add_option("-e,--encounter-id", encounterId, "Encounter ID to add module to (optional, creates new encounter if not provided)");
    addCmd->add_option("-p,--password", password, "Password for encrypted UMDF file (optional)");
    addCmd->add_option("-a,--author", author, "Author of the UMDF file (default: 'User (Default)')");

    CLI::App* updateCmd = writeCmd->add_subcommand("update", "Update an existing module in a UMDF file");
    updateCmd->add_option("-i,--input", inputFile, "Input mock data file")->required();
    updateCmd->add_option("-o,--output", outputFile, "UMDF file to update")->required();
    updateCmd->add_option("--module-id", encounterId, "Module ID to update")->required();
    updateCmd->add_option("-p,--password", password, "Password for encrypted UMDF file (required if file is encrypted)");
    updateCmd->add_option("-a,--author", author, "Author name for audit trail (required)");

    CLI::App* addVariantCmd = writeCmd->add_subcommand("addVariant", "Add a variant module to an existing parent module");
    addVariantCmd->add_option("-i,--input", inputFile, "Input mock data file")->required();
    addVariantCmd->add_option("-o,--output", outputFile, "UMDF file to update")->required();
    addVariantCmd->add_option("--module-id", encounterId, "Parent module ID to add variant to")->required();
    addVariantCmd->add_option("-p,--password", password, "Password for encrypted UMDF file (required if file is encrypted)");
    addVariantCmd->add_option("-a,--author", author, "Author name for audit trail (required)");

    CLI::App* addAnnotationCmd = writeCmd->add_subcommand("addAnnotation", "Add an annotation module to an existing parent module");
    addAnnotationCmd->add_option("-i,--input", inputFile, "Input mock data file")->required();
    addAnnotationCmd->add_option("-o,--output", outputFile, "UMDF file to update")->required();
    addAnnotationCmd->add_option("--module-id", encounterId, "Parent module ID to add annotation to")->required();
    addAnnotationCmd->add_option("-p,--password", password, "Password for encrypted UMDF file (required if file is encrypted)");
    addAnnotationCmd->add_option("-a,--author", author, "Author name for audit trail (required)");

    // READ subcommand
    CLI::App* readCmd = app->add_subcommand("read", "Read and display data from a UMDF file");
    readCmd->add_option("-i,--input", inputFile, "Input UMDF file")->required();
    readCmd->add_option("-p,--password", password, "Password for encrypted UMDF file (required if file is encrypted)");
    readCmd->add_option("-a,--author", author, "Author name for audit trail (optional)");

    // Require that one subcommand is given
    app->require_subcommand();
    
    return {demoCmd, writeCmd, readCmd, createCmd, addCmd, updateCmd, addVariantCmd, addAnnotationCmd};
}


void addDemoOptions(CLI::App* demoCmd, string& outputFile) {

    demoCmd->add_option("-o,--output", outputFile, "Output UMDF file (optional, defaults to demo.umdf)");

    demoCmd->callback([&]() {
        // Set default filename if not provided
        if (outputFile.empty()) {
            outputFile = "demo.umdf";
        }
    });
}


// Helper function to display module data
void displayModuleData(ModuleData& moduleData, const string& moduleType, const string& moduleUuid) {
    cout << "Module: " << moduleType << " (UUID: " << moduleUuid << ")" << endl;
    cout << "Metadata: " << moduleData.metadata.dump(2) << endl;
                
                // Handle the data variant based on type
    const auto& data = moduleData.data;
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
        for (size_t i = 0; i < nestedModules.size() && i < 3; i++) { // Limit to first 2 for readability
                        cout << "  Sub-module " << i << " metadata: " 
                             << nestedModules[i].metadata.dump(2) << endl;
                    }
                    if (nestedModules.size() > 3) {
                        cout << "  ... and " << (nestedModules.size() - 3) << " more sub-modules" << endl;
                    }
                }
                cout << endl;
            }

// Helper function to display module tree with relationships
void displayModuleTree(Reader& reader, const nlohmann::json& moduleTree, int indentLevel) {
    for (const auto& module : moduleTree) {
        // Indent based on level
        string indent(indentLevel * 2, ' ');
        
        cout << indent << "\n└─ Module: " << module["id"] << "\n";
        
        // Display relationships FIRST to make them prominent
        if (module.contains("annotated_by")) {
            cout << indent << "    ANNOTATIONS:\n";
            for (const auto& annotation : module["annotated_by"]) {
                cout << indent << "      └─ " << annotation["id"] << "\n";
            }
            cout << "\n";
        }
        
        if (module.contains("variant")) {
            cout << indent << "    VARIANTS:\n";
            for (const auto& variant : module["variant"]) {
                cout << indent << "      └─ " << variant["id"] << "\n";
            }
            cout << "\n";
        }
        
        // Get and display module data
        auto moduleData = reader.getModuleData(module["id"]);
        if (moduleData) {
            // Display full metadata
            cout << indent << "   Metadata: " << moduleData.value().metadata.dump(2) << "\n";
            
            // Display full data based on type
            const auto& data = moduleData.value().data;
            if (std::holds_alternative<nlohmann::json>(data)) {
                // Tabular data
                cout << indent << "   Data (Tabular): " << std::get<nlohmann::json>(data).dump(2) << "\n";
            } else if (std::holds_alternative<std::vector<uint8_t>>(data)) {
                // Binary data (like image pixels)
                const auto& binaryData = std::get<std::vector<uint8_t>>(data);
                cout << indent << "   Data (Binary): " << binaryData.size() << " bytes\n";
            } else if (std::holds_alternative<std::vector<ModuleData>>(data)) {
                // Nested modules (like image frames)
                const auto& nestedModules = std::get<std::vector<ModuleData>>(data);
                cout << indent << "   Data (Nested): " << nestedModules.size() << " sub-modules\n";
                
                // Show details of first few nested modules
                for (size_t i = 0; i < nestedModules.size() && i < 2; i++) {
                    cout << indent << "     Sub-module " << i << " metadata: " 
                         << nestedModules[i].metadata.dump(2) << "\n";
                }
                if (nestedModules.size() > 2) {
                    cout << indent << "     ... and " << (nestedModules.size() - 2) << " more sub-modules\n";
                }
            }
        }
        
        cout << "\n";
    }
}

// Helper function to display file data (reusable for demo and read commands)
void displayFileData(Reader& reader, const nlohmann::json& fileInfo, bool showSummary) {
    if (showSummary) {
        cout << "Final file statistics:\n";
        cout << "  Total modules: " << fileInfo["module_count"] << "\n";
        
        // List all modules
        if (fileInfo.contains("modules")) {
            cout << "  Module breakdown:\n";
            for (const auto& module : fileInfo["modules"]) {
                cout << "    - " << module["type"] << " data (UUID: " << module["uuid"] << ")\n";
            }
        }

        cout << "Module graph:" << endl;
        cout << fileInfo["module_graph"].dump(2) << endl;
    }

    cout << "\nDISPLAYING DATA BY ENCOUNTER AND MODULE RELATIONSHIPS\n";
    cout << string(60, '-') << "\n";

    // Display data organized by encounters
    if (fileInfo.contains("module_graph") && fileInfo["module_graph"].contains("encounters")) {
        const auto& encounters = fileInfo["module_graph"]["encounters"];
        
        for (size_t encounterIdx = 0; encounterIdx < encounters.size(); encounterIdx++) {
            const auto& encounter = encounters[encounterIdx];
            cout << "\nENCOUNTER " << (encounterIdx + 1) << ": " << encounter["encounter_id"] << "\n";
            cout << string(50, '-') << "\n";
            
            if (encounter.contains("module_tree")) {
                displayModuleTree(reader, encounter["module_tree"], 0);
            }
        }
    } else {
        // Fallback to original display if no encounter structure
        cout << "No encounter structure found, displaying modules individually:\n\n";
        if (fileInfo.contains("modules")) {
            for (const auto& module : fileInfo["modules"]) {
                auto moduleData = reader.getModuleData(module["uuid"]);
                if (moduleData) {
                    displayModuleData(moduleData.value(), module["type"], module["uuid"]);
                }
            }
        }
    }
}

void displayOperationHeader(const string& operation, const string& inputFile, const string& outputFile, const string& encounterId, const string& author, const string& password) {
    cout << "=== " << operation << " ===" << endl;
    cout << "Input mock data: " << inputFile << endl;
    cout << "Output file: " << outputFile << endl;
    if (!encounterId.empty()) {
        cout << "Encounter ID: " << encounterId << endl;
    } else {
        cout << "Encounter ID: (will create new encounter)" << endl;
    }
    cout << "Author: " << author << endl;
    displayEncryptionStatus(password);
    cout << endl;
}

void displayEncryptionStatus(const string& password) {
    if (password != "") {
        cout << "Encryption: AES-256-GCM enabled" << endl;
    } else {
        cout << "Encryption: NONE" << endl;
    }
}

bool loadMockData(const string& inputFile, string& schemaPath, ModuleData& moduleData) {
    try {
        auto mockDataPair = MockDataLoader::loadMockData(inputFile);
        schemaPath = mockDataPair.first;
        moduleData = mockDataPair.second;
        cout << "Loaded mock data with schema: " << schemaPath << endl;
        return true;
    } catch (const std::exception& e) {
        cerr << "Failed to load mock data: " << e.what() << endl;
        return false;
    }
}

bool openOrCreateFile(Writer& writer, const string& operation, string& outputFile, const string& author, const string& password) {
    if (operation == "create") {
        cout << "Creating new UMDF file: " << outputFile << endl;
        Result result = writer.createNewFile(outputFile, author, password);
        if (!result.success) {
            cerr << "Failed to create file: " << result.message << endl;
            return false;
        }
    } else {
        cout << "Opening existing UMDF file: " << outputFile << endl;
        Result result = writer.openFile(outputFile, author, password);
        if (!result.success) {
            cerr << "Failed to open file: " << result.message << endl;
            return false;
        }
    }
    return true;
}

bool closeFile(Writer& writer) {
    auto closeResult = writer.closeFile();
    if (!closeResult.success) {
        cerr << "Failed to close file: " << closeResult.message << endl;
        return false;
    }
    return true;
}