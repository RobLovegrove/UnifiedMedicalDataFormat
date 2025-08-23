#include "writer.hpp"
#include "reader.hpp"
#include "Xref/xref.hpp"
#include "DataModule/dataModule.hpp"
#include "DataModule/Image/imageData.hpp"
#include "DataModule/Tabular/tabularData.hpp"
#include "Utility/Compression/ZstdCompressor.hpp"


#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <expected>
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace std;

/* ================================================== */
/* =============== WRITING OPERATIONS =============== */
/* ================================================== */

Result Writer::createNewFile(std::string& filename) {

    newFile = true;

    //Check file stream is not already open
    if (fileStream.is_open()) {
        return Result{false, "A file is already open"};
    }

    // Check if file exists and is not empty or currently open
    if (std::filesystem::exists(filename)) {
        return Result{false, "Trying to create new file, but file already exists"};
    }

    // Set up file stream
    Result result = setUpFileStream(filename);
    if (!result.success) {
        return result;
    }

    // Ensure at the start of the file
    fileStream.seekp(0);

    // Write header to file
    try {
        if (!header.writePrimaryHeader(fileStream)) {
            fileStream.close();
            removeTempFile();
            return Result{false, "Failed to write header to temp file"};
        }
    } catch (const std::exception& e) {
        fileStream.close();
        removeTempFile();
        return Result{false, "Exception writing header: " + std::string(e.what())};
    }

    return Result{true, "File created successfully"};
}

Result Writer::openFile(std::string& filename) {

    // Check if file stream is already open
    if (fileStream.is_open()) {
        return Result{false, "A file is already open"};
    }

    // Check if file exists
    if (!std::filesystem::exists(filename)) {
        return Result{false, "File does not exist"};
    }

    // Check if file is empty
    if (std::filesystem::is_empty(filename)) {
        return Result{false, "File is empty"};
    }

    Result result = setUpFileStream(filename);
    if (!result.success) {
        return result;
    }

    // Read header from temp file
    if (!header.readPrimaryHeader(fileStream)) {
        fileStream.close();
        removeTempFile();
        return Result{false, "Failed to read header from temp file"};
    }
    
    // Load XRef table from temp file
    xrefTable.clear();
    try {
        xrefTable = XRefTable::loadXrefTable(fileStream);
    } catch (const std::exception& e) {
        fileStream.close();
        removeTempFile();
        return Result{false, "Failed to load XRef table from temp file: " + std::string(e.what())};
    }

    return Result{true, "File opened successfully"};
}

std::expected<UUID, std::string> Writer::addModule(const std::string& schemaPath, const ModuleData& module) {
    
    UUID moduleId;
    
    // Check if file stream is open
    if (!fileStream.is_open()) {
        return std::unexpected("No file is open");
    }

    try {
        auto result = writeModule(fileStream, schemaPath, module, header.getEncryptionData());
        if (!result) {
            return std::unexpected(result.error());
        }
        moduleId = result.value();

    } catch (const std::exception& e) {
        return std::unexpected("Exception writing module: " + std::string(e.what()));
    }

    return moduleId;
}

Result Writer::updateModule(const std::string& moduleId, const ModuleData& module) {

    // Check if file stream is open
    if (!fileStream.is_open()) {
        return Result{false, "No file is open"};
    }

    auto& entries = xrefTable.getEntries();
    for (auto it = entries.begin(); it != entries.end(); ) {
        if (it->id.toString() == moduleId) {
            // Go to module offset in file
            fileStream.seekg(it->offset);
    
            // Create the DataHeader
            DataHeader dataHeader;
            dataHeader.setEncryptionData(header.getEncryptionData());
            dataHeader.readDataHeader(fileStream);
    
            // Return to the start of the module
            fileStream.seekg(it->offset);
    
            // Update the module's isCurrent flag to false
            dataHeader.updateIsCurrent(false, fileStream);
    
            // Write the new module data
            unique_ptr<DataModule> dm;
    
            switch (dataHeader.getModuleType()) {
                case ModuleType::Image: {
                    dm = make_unique<ImageData>(dataHeader.getSchemaPath(), dataHeader.getModuleID(), dataHeader.getEncryptionData());
                    break;
                }
                case ModuleType::Tabular: {
                    dm = make_unique<TabularData>(dataHeader.getSchemaPath(), dataHeader.getModuleID(), dataHeader.getEncryptionData());
                    break;
                }
                default:

                    return Result{false, "Invalid module type"};
            }
    
            // Set the previous offset as the offset of the old module 
            dm->setPrevious(it->offset);
    
            // Delete the old module from the xref table
            it = entries.erase(it);
    
            dm->addMetaData(module.metadata);
            dm->addData(module.data);
    
            // Ensure at end of file
            fileStream.seekp(0, std::ios::end);

            streampos moduleStart = fileStream.tellp();
    
            std::stringstream moduleBuffer;
            dm->writeBinary(moduleStart, moduleBuffer, xrefTable);

            string bufferData = moduleBuffer.str();
            fileStream.write(reinterpret_cast<char*>(bufferData.data()), bufferData.size());

            // Since we found and processed the module, we can break
            break;
        } else {
            ++it;  // Move to next element
        }
    }

    return Result{true, "Module updated successfully"};
}

Result Writer::closeFile() {

    // Check if file stream is open
    if (!fileStream.is_open()) {
        return Result{false, "No file is open"};
    }

    // Check XrefTable is not empty
    if (xrefTable.getEntries().empty()) {
        // No modules to write so delete temp file
        std::filesystem::remove(tempFilePath);

        resetWriter();
        return Result{true, "File closed successfully"};
    }

    // Check temp file exists
    if (!std::filesystem::exists(tempFilePath)) {
        return Result{false, "Temp file does not exist"};
    }

    // Check temp file is not empty
    if (std::filesystem::is_empty(tempFilePath)) {
        return Result{false, "Temp file is empty"};
    }

    // Make old XREF table obsolete and write new one
    try {
        if (!newFile) {
            xrefTable.setObsolete(fileStream);
        }

        if (!writeXref(fileStream)) {
            return Result{false, "Failed to write XREF table to temp file"};
        }
    } catch (const std::exception& e) {
        fileStream.close();
        std::filesystem::remove(tempFilePath);
        return Result{false, "Exception writing XREF table: " + std::string(e.what())};
    }

    // Close file stream
    fileStream.close();

    // Validate temp file
    try {
        auto result = validateTempFile();
        if (!result.success) {
            removeTempFile();
            return result;
        } else {
            cout << "Temp file validated successfully" << endl;
        }
    } catch (const std::exception& e) {
        removeTempFile();
        return Result{false, "Exception during temp file validation: " + std::string(e.what())};
    }

    // Atomic replace (rename temp to original)
    cout << "Renaming temp file: " << tempFilePath << " to: " << filePath << endl;
    try {
        std::filesystem::rename(tempFilePath, filePath);
        cout << "File renamed successfully" << endl;
    } catch (const std::exception& e) {
        std::filesystem::remove(tempFilePath);
        return Result{false, "Exception renaming temp file: " + std::string(e.what())};
    }

    newFile = false;

    return Result{true, "File closed successfully"};
}

void Writer::resetWriter() {
    fileStream.close();
    header = Header();
    xrefTable.clear();
    tempFilePath.clear();
    filePath.clear();
}

Result Writer::setUpFileStream(std::string& filename) {

    resetWriter();

    filePath = filename;
    tempFilePath = filename + ".tmp";

    // Create new temp file
    if (!newFile) {
        // Copy original file to temp file
        if (std::filesystem::exists(tempFilePath)) {
            std::filesystem::remove(tempFilePath);
        }
        std::filesystem::copy_file(filename, tempFilePath);

        fileStream.open(tempFilePath, std::ios::binary | std::ios::in | std::ios::out);
        if (!fileStream) {   
            return Result{false, "Failed to open temp file: " + std::string(std::strerror(errno))};
        }
    }
    else {    
        fileStream.open(tempFilePath, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!fileStream) {   
            cout << "Failing here" << endl;
            return Result{false, "Failed to open temp file: " + std::string(std::strerror(errno))};
        }
    }

    // SET ENCRYPTION HEADER
    EncryptionData encryptionData;
    encryptionData.masterPassword = "password";
    encryptionData.encryptionType = EncryptionType::AES_256_GCM;
    //encryptionData.encryptionType = EncryptionType::NONE;
    encryptionData.baseSalt = EncryptionManager::generateSalt(16); // Use correct Argon2id salt size
    encryptionData.memoryCost = 65536;
    encryptionData.timeCost = 3;
    encryptionData.parallelism = 1;

    header.setEncryptionData(encryptionData);

    return Result{true, "File stream set up successfully"};
}



std::expected<std::vector<UUID>, std::string> Writer::writeNewFile(std::string& filename, 
    std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas) {

    std::expected<UUID, std::string> result;
    std::vector<UUID> moduleIds;

    // Set up file paths
    tempFilePath = filename + ".tmp";
    
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

    // SET ENCRYPTION HEADER
    EncryptionData encryptionData;
    encryptionData.masterPassword = "password";
    encryptionData.encryptionType = EncryptionType::AES_256_GCM;
    //encryptionData.encryptionType = EncryptionType::NONE;
    encryptionData.baseSalt = EncryptionManager::generateSalt(16); // Use correct Argon2id salt size
    encryptionData.memoryCost = 65536;
    encryptionData.timeCost = 3;
    encryptionData.parallelism = 1;

    header.setEncryptionData(encryptionData);

    try {
        if (!header.writePrimaryHeader(tempFile)) {
            tempFile.close();
            removeTempFile();
            return std::unexpected("Failed to write header to temp file");
        }
    } catch (const std::exception& e) {
        tempFile.close();
        removeTempFile();
        return std::unexpected("Exception writing header: " + std::string(e.what()));
    }

    // Write Modules to file
    try {
        // CREATE MODULES 
        for (size_t i = 0; i < modulesWithSchemas.size(); ++i) {
            const auto& [schemaPath, moduleData] = modulesWithSchemas[i];



            // Write module to temp file
            result = writeModule(tempFile, schemaPath, moduleData, header.getEncryptionData());
            if (!result) {
                tempFile.close();
                removeTempFile();
                return std::unexpected(result.error());
            }
            moduleIds.push_back(result.value());

        }

    } catch (const std::exception& e) {
        tempFile.close();
        removeTempFile();
        return std::unexpected("Exception writing modules: " + std::string(e.what()));
    }

    // Write XREF table to temp file
    try {
        if (!writeXref(tempFile)) {
            tempFile.close();
            removeTempFile();
            return std::unexpected("Failed to write xref table to temp file");
        }
    } catch (const std::exception& e) {
        tempFile.close();
        removeTempFile();
        return std::unexpected("Exception writing xref table: " + std::string(e.what()));
    }



    // Close temp file
    try {
        tempFile.close();
    } catch (const std::exception& e) {
        removeTempFile();
        return std::unexpected("Exception closing temp file: " + std::string(e.what()));
    }




    try {
        auto result = validateTempFile(modulesWithSchemas.size());
        if (!result.success) {
            removeTempFile();
            return std::unexpected(result.message);
        }
    } catch (const std::exception& e) {
        removeTempFile();
        return std::unexpected("Exception during temp file validation: " + std::string(e.what()));
    }



    // Rename temp file to new file

    try {
        if (!renameTempFile(filename)) {
            removeTempFile();
            return std::unexpected("Failed to rename temp file");
        }
    } catch (const std::exception& e) {
        removeTempFile();
        return std::unexpected("Exception renaming temp file: " + std::string(e.what()));
    }



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

    // Read header from temp file
    if (!header.readPrimaryHeader(tempFile)) {
        tempFile.close();
        std::filesystem::remove(tempFilePath);
        return std::unexpected("Failed to read header from temp file");
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
            // Write module to temp file
            result = writeModule(tempFile, schemaPath, moduleData, header.getEncryptionData());
            if (!result) {

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
        auto result = validateTempFile();
        if (!result.success) {
            std::filesystem::remove(tempFilePath);
            return std::unexpected(result.message);
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

    // Read header from temp file
    if (!header.readPrimaryHeader(tempFile)) {
        tempFile.close();
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
        
                // Create the DataHeader
                DataHeader dataHeader;
                dataHeader.setEncryptionData(header.getEncryptionData());
                dataHeader.readDataHeader(tempFile);
        
                // Return to the start of the module
                tempFile.seekg(it->offset);
        
                // Update the module's isCurrent flag to false
                dataHeader.updateIsCurrent(false, tempFile);
        
                // Write the new module data
                unique_ptr<DataModule> dm;
        
                switch (dataHeader.getModuleType()) {
                    case ModuleType::Image: {
                        dm = make_unique<ImageData>(dataHeader.getSchemaPath(), dataHeader.getModuleID(), dataHeader.getEncryptionData());
                        break;
                    }
                    case ModuleType::Tabular: {
                        dm = make_unique<TabularData>(dataHeader.getSchemaPath(), dataHeader.getModuleID(), dataHeader.getEncryptionData());
                        break;
                    }
                    default:

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
        auto result = validateTempFile();
        if (!result.success) {
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
    std::ostream& outfile, const std::string& schemaPath, const ModuleData& moduleData, EncryptionData encryptionData) {

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
    
    // CREATE MODULE
    switch (type) {
        case ModuleType::Image: {
            dm = make_unique<ImageData>(schemaPath, schemaJson, uuid, encryptionData);
            break;
        }
        case ModuleType::Tabular: {
            dm = make_unique<TabularData>(schemaPath, schemaJson, uuid, encryptionData);
            break;
        }
        default:

            return std::unexpected("Unknown module type: " + moduleType);
    }

    // ADD DATA TO MODULE
    dm->addMetaData(moduleData.metadata);
    dm->addData(moduleData.data);

    // Ensure at end of file
    outfile.seekp(0, std::ios::end);

    streampos moduleStart = outfile.tellp();

    // Start ZSTD summary mode for this module
    ZstdCompressor::startSummaryMode();

    // WRITE MODULE TO FILE
    std::stringstream moduleBuffer;
    dm->writeBinary(moduleStart, moduleBuffer, xrefTable);

    string bufferData = moduleBuffer.str();
    outfile.write(reinterpret_cast<char*>(bufferData.data()), bufferData.size());

    // Print ZSTD compression summary for this module and stop summary mode
    if (ZstdCompressor::isSummaryMode()) {
        std::cout << "Module ZSTD compression summary:" << std::endl;
        ZstdCompressor::printSummary();
        ZstdCompressor::stopSummaryMode();
    }

    return dm->getModuleID();
}

void Writer::removeTempFile() {
    if (std::filesystem::exists(tempFilePath)) {
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

Result Writer::validateTempFile(size_t moduleCount) {

    // Check if temp file exists
    if (!std::filesystem::exists(tempFilePath)) {
        return Result{false, "Temp file does not exist"};
    }
    
    // Open temp file
    std::ifstream tempFile(tempFilePath, std::ios::binary);
    if (!tempFile) return Result{false, "Failed to open temp file during validation"};

    // Check if temp file is not empty
    if (std::filesystem::is_empty(tempFilePath)) {
        return Result{false, "Temp file is empty"};
    }

    // Read header from temp file
    if (!header.readPrimaryHeader(tempFile)) {
        tempFile.close();
        return Result{false, "Failed to read header from temp file during validation"};
    }

    // Load XRef table from temp file
    xrefTable = XRefTable::loadXrefTable(tempFile);

    // Check module count
    if (moduleCount > 0) {
        if (xrefTable.getEntries().size() != moduleCount) {
            return Result{false, "Module count mismatch during validation"};
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

            return Result{false, "Failed to read DataHeader from temp file during validation"};
        }
    }

    return Result{true, "Temp file validated successfully"};
}