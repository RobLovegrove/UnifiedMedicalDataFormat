#include <iostream>
#include <string>

#include "writer.hpp"
#include "reader.hpp"
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
    Reader reader;

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
        
        // Read patient data back
        cout << "Reading from file: " << inputFile << "\n";
        //string readData;
        if (!reader.readFile(inputFile)) {
            cerr << "Failed to read data\n";
            return 1;
        }
        //cout << "Read patient data:\n" << readData << "\n";
        cout << "File read complete" << endl;
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