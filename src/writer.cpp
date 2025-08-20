#include "writer.hpp"
#include "reader.hpp"
#include "Xref/xref.hpp"
#include "DataModule/dataModule.hpp"
#include "DataModule/Image/imageData.hpp"
#include "DataModule/Tabular/tabularData.hpp"


#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <expected>
#include <nlohmann/json.hpp>

using namespace std;

/* ================================================== */
/* =============== WRITING OPERATIONS =============== */
/* ================================================== */

std::expected<std::vector<UUID>, std::string> Writer::writeNewFile(std::string& filename, 
    std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas) {

    std::expected<UUID, std::string> result;
    std::vector<UUID> moduleIds;

    // Set up file paths
    tempFilePath = filename + ".tmp";

    cout << "Opening tmp file: " << tempFilePath << endl;
    
    // Open temp file for writing
    std::ofstream tempFile;
    try {
        tempFile.open(tempFilePath, std::ios::binary);
        if (!tempFile) {
            return std::unexpected("Failed to create temp file: " + tempFilePath);
        }
    } catch (const std::exception& e) {
        return std::unexpected("Exception opening temp file: " + std::string(e.what()));
    }

    cout << "Writing new file: " << filename << endl;
    cout << "Using temp file: " << tempFilePath << endl;

    // CREATE HEADER
    Header header;
    try {
        if (!header.writePrimaryHeader(tempFile)) {
            tempFile.close();
            cleanupTempFile();
            return std::unexpected("Failed to write header to temp file");
        }
    } catch (const std::exception& e) {
        tempFile.close();
        cleanupTempFile();
        return std::unexpected("Exception writing header: " + std::string(e.what()));
    }

    cout << "Header written to temp file" << endl;

    // Write Modules to file
    try {
        // CREATE MODULES 
        for (size_t i = 0; i < modulesWithSchemas.size(); ++i) {
            const auto& [schemaPath, moduleData] = modulesWithSchemas[i];

            cout << "Processing module " << (i + 1) << "/" << modulesWithSchemas.size() << endl;

            // Write module to temp file
            result = writeModule(tempFile, schemaPath, moduleData);
            if (!result) {
                tempFile.close();
                cleanupTempFile();
                return std::unexpected(result.error());
            }
            moduleIds.push_back(result.value());

        }

    } catch (const std::exception& e) {
        tempFile.close();
        cleanupTempFile();
        return std::unexpected("Exception writing modules: " + std::string(e.what()));
    }

    cout << "All modules written to temp file" << endl;

    // Write XREF table to temp file
    try {
        if (!writeXref(tempFile)) {
            tempFile.close();
            cleanupTempFile();
            return std::unexpected("Failed to write xref table to temp file");
        }
    } catch (const std::exception& e) {
        tempFile.close();
        cleanupTempFile();
        return std::unexpected("Exception writing xref table: " + std::string(e.what()));
    }

    cout << "XREF table written to temp file" << endl;

    // Close temp file
    try {
        tempFile.close();
    } catch (const std::exception& e) {
        cleanupTempFile();
        return std::unexpected("Exception closing temp file: " + std::string(e.what()));
    }

    cout << "Temp file closed" << endl;


    try {
        if (!validateTempFile(modulesWithSchemas.size())) {
            cleanupTempFile();
            return std::unexpected("Temp file validation failed");
        }
    } catch (const std::exception& e) {
        cleanupTempFile();
        return std::unexpected("Exception during temp file validation: " + std::string(e.what()));
    }

    cout << "Temp file validated" << endl;

    // Rename temp file to new file

    try {
        if (!renameTempFile(filename)) {
            cleanupTempFile();
            return std::unexpected("Failed to rename temp file");
        }
    } catch (const std::exception& e) {
        cleanupTempFile();
        return std::unexpected("Exception renaming temp file: " + std::string(e.what()));
    }

    cout << "File committed successfilly: " << filename << endl;

    return moduleIds;
}

std::expected<std::vector<UUID>, std::string> Writer::addModules(
    std:: string& filename, std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas) {

    std::expected<UUID, std::string> result;
    std::vector<UUID> moduleIds;

    tempFilePath = filename + ".tmp";

    // Check if file exists and is not empty or currently open
    if (!std::filesystem::exists(filename) || std::filesystem::is_empty(filename)) {
        return std::unexpected("File does not exist or is empty");
    }

    // Copy original file to temp file
    try {
        // Remove temp file if it exists
        if (std::filesystem::exists(tempFilePath)) {
            std::filesystem::remove(tempFilePath);
        }
        // Copy original to temp
        std::filesystem::copy_file(filename, tempFilePath);
    } catch (const std::exception& e) {
        return std::unexpected("Failed to copy original file: " + std::string(e.what()));
    }

    cout << "Opening tmp file: " << tempFilePath << endl;

    // Open temp file for writing
    std::fstream tempFile;
    try {
        tempFile.open(tempFilePath, std::ios::in | std::ios::out | std::ios::binary);
        if (!tempFile) {
            std::filesystem::remove(tempFilePath);
            return std::unexpected("Failed to open temp file for updating");
        }
    } catch (const std::exception& e) {
        std::filesystem::remove(tempFilePath);
        return std::unexpected("Exception opening temp file: " + std::string(e.what()));
    }
    
    // Load XRef table from temp file
    xrefTable.clear();
    try {
        xrefTable = XRefTable::loadXrefTable(tempFile);
    } catch (const std::exception& e) {
        tempFile.close();
        std::filesystem::remove(tempFilePath);
        return std::unexpected("Failed to load XRef table from temp file: " + std::string(e.what()));
    }


    try {
        // Write Modules to file
        for (size_t i = 0; i < modulesWithSchemas.size(); ++i) {
            const auto& [schemaPath, moduleData] = modulesWithSchemas[i];

            cout << "Processing module " << (i + 1) << "/" << modulesWithSchemas.size() << endl;

            // Write module to temp file
            result = writeModule(tempFile, schemaPath, moduleData);
            if (!result) {
                cout << "Failed to write module: " << result.error() << endl;
                return std::unexpected(result.error());
            }
            moduleIds.push_back(result.value());
        }

    } catch (const std::exception& e) {
        tempFile.close();
        std::filesystem::remove(tempFilePath);
        return std::unexpected("Exception writing modules: " + std::string(e.what()));
    }

    cout << "All new modules written to temp file" << endl;

    // Make old XREF table obsolete and write new one
    try {
        xrefTable.setObsolete(tempFile);
        
        if (!writeXref(tempFile)) {
            throw std::runtime_error("Failed to write new XRef table");
        }
    } catch (const std::exception& e) {
        tempFile.close();
        std::filesystem::remove(tempFilePath);
        return std::unexpected("Exception updating XRef table: " + std::string(e.what()));
    }

    // Close temp file
    tempFile.close();

    // Validate temp file
    try {
        if (!validateTempFile()) {
            std::filesystem::remove(tempFilePath);
            return std::unexpected("Updated file validation failed");
        }
    } catch (const std::exception& e) {
        std::filesystem::remove(tempFilePath);
        return std::unexpected("Exception during validation: " + std::string(e.what()));
    }

    // Atomic replace (rename temp to original)
    try {
        std::filesystem::rename(tempFilePath, filename);
    } catch (const std::exception& e) {
        std::filesystem::remove(tempFilePath);
        return std::unexpected("Failed to replace original file: " + std::string(e.what()));
    }

    cout << "File updated successfully: " << filename << endl;

    return moduleIds;
}

bool Writer::updateModules(
    std::string& filename,
    std::vector<std::pair<std::string, ModuleData>>& moduleUpdates) {

    tempFilePath = filename + ".tmp";

    // Check if file exists and is not empty or currently open
    if (!std::filesystem::exists(filename) || std::filesystem::is_empty(filename)) {
        return false;
    }

    // Copy original file to temp file
    try {
        // Remove temp file if it exists
        if (std::filesystem::exists(tempFilePath)) {
            std::filesystem::remove(tempFilePath);
        }
        // Copy original to temp
        std::filesystem::copy_file(filename, tempFilePath);
    } catch (const std::exception& e) {
        return false;
    }

    cout << "Opening tmp file: " << tempFilePath << endl;

    // Open temp file for writing
    std::fstream tempFile;
    try {
        tempFile.open(tempFilePath, std::ios::in | std::ios::out | std::ios::binary);
        if (!tempFile) {
            std::filesystem::remove(tempFilePath);
            return false;
        }
    } catch (const std::exception& e) {
        std::filesystem::remove(tempFilePath);
        return false;
    }
    
    // Load XRef table from temp file
    xrefTable.clear();
    try {
        xrefTable = XRefTable::loadXrefTable(tempFile);
    } catch (const std::exception& e) {
        tempFile.close();
        std::filesystem::remove(tempFilePath);
        return false;
    }

    // Update modules in temp file
    for (const auto& [moduleId, moduleData] : moduleUpdates) {

        auto& entries = xrefTable.getEntries();  // Get reference to avoid copying
        for (auto it = entries.begin(); it != entries.end(); ) {
            if (it->id.toString() == moduleId) {
                // Go to module offset in file
                tempFile.seekg(it->offset);
        
                // Read the DataHeader
                DataHeader dataHeader;
                dataHeader.readDataHeader(tempFile);
        
                // Return to the start of the module
                tempFile.seekg(it->offset);
        
                // Update the module's isCurrent flag to false
                dataHeader.updateIsCurrent(false, tempFile);
        
                // Write the new module data
                unique_ptr<DataModule> dm;
        
                switch (dataHeader.getModuleType()) {
                    case ModuleType::Image: {
                        dm = make_unique<ImageData>(dataHeader.getSchemaPath(), dataHeader.getModuleID());
                        break;
                    }
                    case ModuleType::Tabular: {
                        dm = make_unique<TabularData>(dataHeader.getSchemaPath(), dataHeader.getModuleID());
                        break;
                    }
                    default:
                        cout << "Unknown module type: " << dataHeader.getModuleType() << endl;
                        return false;
                }
        
                // Set the previous offset as the offset of the old module 
                dm->setPrevious(it->offset);
        
                // Delete the old module from the xref table
                it = entries.erase(it);
        
                dm->addMetaData(moduleData.metadata);
                dm->addData(moduleData.data);
        
                // Ensure at end of file
                tempFile.seekp(0, std::ios::end);

                streampos moduleStart = tempFile.tellp();
        
                std::stringstream moduleBuffer;
                dm->writeBinary(moduleStart, moduleBuffer, xrefTable);

                string bufferData = moduleBuffer.str();
                tempFile.write(reinterpret_cast<char*>(bufferData.data()), bufferData.size());

                // Since we found and processed the module, we can break
                break;
            } else {
                ++it;  // Move to next element
            }
        }

    }

    // Make old XREF table obsolete and write new one
    try {
        xrefTable.setObsolete(tempFile);
        
        if (!writeXref(tempFile)) {
            throw std::runtime_error("Failed to write new XRef table");
        }
    } catch (const std::exception& e) {
        tempFile.close();
        std::filesystem::remove(tempFilePath);
        return false;
    }

    // Close temp file
    tempFile.close();

    // Validate temp file
    try {
        if (!validateTempFile()) {
            std::filesystem::remove(tempFilePath);
            return false;
        }
    } catch (const std::exception& e) {
        std::filesystem::remove(tempFilePath);
        return false;
    }

    // Atomic replace (rename temp to original)
    try {
        std::filesystem::rename(tempFilePath, filename);
    } catch (const std::exception& e) {
        std::filesystem::remove(tempFilePath);
        return false;
    }

    cout << "File updated successfully: " << filename << endl;

    return true;
}

/* ======================================================= */
/* =============== WRITER HELPER FUNCTIONS =============== */
/* ======================================================= */

bool Writer::writeXref(std::ostream& outfile) { 
    // Explicitly seek to end of file
    outfile.seekp(0, std::ios::end);
    
    uint64_t offset = outfile.tellp();  // Get position at end
    xrefTable.setXrefOffset(offset);
    return xrefTable.writeXref(outfile);
}

std::expected<UUID, std::string> Writer::writeModule(
    std::ostream& outfile, const std::string& schemaPath, const ModuleData& moduleData) {

    // Load schema from file
    std::ifstream schemaFile(schemaPath);
    if (!schemaFile.is_open()) {
        cerr << "Failed to open schema file: " << schemaPath << endl;
        return std::unexpected("Failed to open schema file");
    }
    
    nlohmann::json schemaJson;
    schemaFile >> schemaJson;
    
    string moduleType = schemaJson["module_type"];
    ModuleType type = module_type_from_string(moduleType);

    UUID uuid = UUID();

    unique_ptr<DataModule> dm;

    cout << "Creating module: " << moduleType << endl;
    
    // CREATE MODULE
    switch (type) {
        case ModuleType::Image: {
            dm = make_unique<ImageData>(schemaPath, schemaJson, uuid);
            break;
        }
        case ModuleType::Tabular: {
            dm = make_unique<TabularData>(schemaPath, schemaJson, uuid);
            break;
        }
        default:
            cout << "Unknown module type: " << moduleType << endl;
            return std::unexpected("Unknown module type: " + moduleType);
    }

    // ADD DATA TO MODULE
    dm->addMetaData(moduleData.metadata);

    cout << "Adding data to module" << endl;
    dm->addData(moduleData.data);

    cout << "Writing module to file" << endl;

    // Ensure at end of file
    outfile.seekp(0, std::ios::end);

    streampos moduleStart = outfile.tellp();

    // WRITE MODULE TO FILE
    std::stringstream moduleBuffer;
    dm->writeBinary(moduleStart, moduleBuffer, xrefTable);

    string bufferData = moduleBuffer.str();
    outfile.write(reinterpret_cast<char*>(bufferData.data()), bufferData.size());

    return dm->getModuleID();
}

void Writer::cleanupTempFile() {
    if (!tempFilePath.empty() && std::filesystem::exists(tempFilePath)) {
        std::filesystem::remove(tempFilePath);
    }
    tempFilePath.clear();
}

bool Writer::renameTempFile(const std::string& finalFilePath) {
    try {
        std::filesystem::rename(tempFilePath, finalFilePath);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception in renameTempFile: " << e.what() << std::endl;
        return false;
    }
}

bool Writer::validateTempFile(size_t moduleCount) {


    cout << "Validating temp file: " << tempFilePath << endl;
    cout << "Module count: " << moduleCount << endl;

    // Check if temp file exists
    if (!std::filesystem::exists(tempFilePath)) {
        return false;
    }
    
    // Check if temp file is not empty
    std::ifstream tempFile(tempFilePath, std::ios::binary);
    if (!tempFile) return false;

    // Read header and confirm UMDF
    if (!header.readPrimaryHeader(tempFile)) { 
        tempFile.close();
        return false; 
    }

    xrefTable = XRefTable::loadXrefTable(tempFile);

    if (moduleCount > 0) {
        if (xrefTable.getEntries().size() != moduleCount) {
            return false;
        }
    }

    for (const auto& entry : xrefTable.getEntries()) {
        // Get entry offset
        uint64_t offset = entry.offset;

        // Seek to entry offset
        tempFile.seekg(offset);

        // Read DataHeader
        DataHeader dataHeader;
        try {
            dataHeader.readDataHeader(tempFile);
        } catch (const std::exception& e) {
            cout << "Exception reading DataHeader: " << e.what() << endl;
            return false;
        }
    }

    return true;
}