#include <iostream>
#include <string>

#include "umdfFile.hpp"
#include "DataModule/dataModule.hpp"
#include "writer.hpp"


#include "CLI11/CLI11.hpp"
#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"

using namespace std;
 

/* -------------------------- CONSTANTS -------------------------- */

/* -------------------------- DECLARATIONS -------------------------- */
void addWriteOptions(
    CLI::App* writeCmd, 
    string& inputFile, 
    string& outputFile, 
    bool& overwrite, 
    bool& update
    );

/* -------------------------- MOCK DATA -------------------------- */


/* -------------------------- MAIN FUNCTION -------------------------- */

int main(int argc, char** argv) {

    UUID uuid;

    CLI::App app{"UMDF - Unified Medical Data Format Tool"};
    
    Writer writer;
    UMDFFile file;

    string inputFile;
    string outputFile;

    bool overwrite = false;
    bool update = false;

    // WRITE subcommand
    CLI::App* writeCmd = app.add_subcommand("write", "Write data to a UMDF file");
    addWriteOptions(writeCmd, inputFile, outputFile, overwrite, update);

    // READ subcommand
    CLI::App* readCmd = app.add_subcommand("read", "Read data from a UMDF file");
    readCmd->add_option("-i,--input", inputFile, "Input UMDF file")->required();

    // Require that one subcommand is given
    app.require_subcommand();

    CLI11_PARSE(app, argc, argv);

    if (*writeCmd) {

        cout << "Writing to file: " << outputFile << "\n";
        if (!writer.writeNewFile(outputFile)) {
            cerr << "Failed to write data\n";
            return 1;
        }

         cout << "Data written successfully.\n";
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
                    
                    // // Optionally show details of nested modules
                    // for (size_t i = 0; i < nestedModules.size(); i++) {
                    //     cout << "  Sub-module " << i << " metadata: " 
                    //          << nestedModules[i].metadata.dump(2) << endl;
                    // }
                }
                cout << endl;
            }
        }
        
        // cout << "File read complete" << endl;
    }

    return 0;
}



/* -------------------------- HELPER FUNCTIONS -------------------------- */

void addWriteOptions(
    CLI::App* writeCmd, 
    string& inputFile, 
    string& outputFile, 
    bool& overwrite, 
    bool& update 
) {

    writeCmd->add_option("-i,--input", inputFile, "Input data file")->required();
    writeCmd->add_option("-o,--output", outputFile, "Output UMDF file")->required();

    writeCmd->add_flag("--overwrite", overwrite, "Overwrite existing UMDF file if it exists");
    writeCmd->add_flag("--update", update, "Update existing UMDF file instead of overwriting");

    writeCmd->callback([&]() {
        if (overwrite && update) {
            throw CLI::ValidationError("Cannot use both --overwrite and --update together.");
        }
    });
}