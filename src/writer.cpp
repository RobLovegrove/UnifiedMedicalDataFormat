#include "writer.hpp"
#include "reader.hpp"
#include "Xref/xref.hpp"
#include "DataModule/dataModule.hpp"
#include "DataModule/Image/imageData.hpp"
#include "DataModule/Tabular/tabularData.hpp"
#include "Utility/Compression/ZstdCompressor.hpp"
#include "Links/moduleGraph.hpp"
#include "Links/moduleLink.hpp"


#include "Utility/utils.hpp"
#include "Utility/uuid.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <expected>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <boost/interprocess/sync/file_lock.hpp>

using namespace std;

/* ================================================== */
/* =============== WRITING OPERATIONS =============== */
/* ================================================== */

Result Writer::createNewFile(std::string& filename, string author, string password) {

    newFile = true;

    //Check file stream is not already open
    if (fileStream.is_open()) {
        return Result{false, "A file is already open"};
    }

    // Check if file exists and is not empty or currently open
    if (std::filesystem::exists(filename)) {
        return Result{false, "Trying to create new file, but file already exists"};
    }

    // Create the file so it can be locked in set up file stream
    {
    std::ofstream touch(filename, std::ios::app);
    }

    // Set up file stream
    Result result = setUpFileStream(filename);
    if (!result.success) {
        return result;
    }

    this->author = author;

    if (password != "") {
        EncryptionData encryptionData;
        encryptionData.masterPassword = password;
        encryptionData.encryptionType = EncryptionType::AES_256_GCM;
        encryptionData.baseSalt = EncryptionManager::generateSalt(16); // Use correct Argon2id salt size
        encryptionData.memoryCost = 65536;
        encryptionData.timeCost = 3;
        encryptionData.parallelism = 2;

        header.setEncryptionData(encryptionData);
    }
    else {
        EncryptionData encryptionData;
        encryptionData.encryptionType = EncryptionType::NONE;
        header.setEncryptionData(encryptionData);
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

Result Writer::openFile(std::string& filename, string author, string password) {

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
        cancelThenClose();
        return result;
    }

    this->author = author;

    // Read header from temp file
    if (!header.readPrimaryHeader(fileStream)) {
        cancelThenClose();
        return Result{false, "Failed to read header from temp file"};
    }

    if (header.getEncryptionData().encryptionType != EncryptionType::NONE) {
        if (password == "") {
            cancelThenClose();
            return Result{false, "File is encrypted but no password provided"};
        }
        else {
            header.setEncryptionPassword(password);
        }
    }
    
    // Load XRef table from temp file
    xrefTable.clear();
    try {
        xrefTable = XRefTable::loadXrefTable(fileStream);
    } catch (const std::exception& e) {
        cancelThenClose();
        return Result{false, "Failed to load XRef table from temp file: " + std::string(e.what())};
    }

    try {
        // Read ModuleGraph
        fileStream.seekg(xrefTable.getModuleGraphOffset());

        std::stringstream buffer;
        std::vector<char> temp(xrefTable.getModuleGraphSize());
        fileStream.read(temp.data(), temp.size());
        buffer.write(temp.data(), temp.size());

        // Read the module graph from the buffer
        moduleGraph = ModuleGraph::readModuleGraph(buffer);

        cout << "Displaying encounters" << endl << endl;
        moduleGraph.displayEncounters();
    }
    catch (const std::exception& e) {
        cancelThenClose();

        return Result{false, "Failed to read ModuleGraph: " + string(e.what())};
    }

    return Result{true, "File opened successfully"};
}

Result Writer::addModule(const std::string& schemaPath, UUID moduleId, const ModuleData& module) {

    
    // Check if file stream is open
    if (!fileStream.is_open()) {
        return Result{false, "No file is open"};
    }

    try {
        auto result = writeModule(fileStream, schemaPath, moduleId, module, header.getEncryptionData());
        if (!result.success) {
            return Result{false, result.message};
        }

    } catch (const std::exception& e) {
        return Result{false, "Exception writing module: " + std::string(e.what())};
    }

    return Result{true, "Module added successfully"};
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
            dataHeader.setModuleID(UUID::fromString(moduleId));

            // Return to the start of the module
            fileStream.seekg(it->offset);
    
            // Update the module's isCurrent flag to false
            dataHeader.updateIsCurrent(false, fileStream);
    
            // Write the new module data
            unique_ptr<DataModule> dm;
    
            switch (dataHeader.getModuleType()) {
                case ModuleType::Image: {
                    dm = make_unique<ImageData>(dataHeader.getSchemaPath(), dataHeader);
                    break;
                }
                case ModuleType::Tabular: {
                    dm = make_unique<TabularData>(dataHeader.getSchemaPath(), dataHeader);
                    break;
                }
                default:

                    return Result{false, "Invalid module type"};
            }
    
            // Set the previous offset as the offset of the old module 
            dm->setPrevious(it->offset);
    
            // // Delete the old module from the xref table - This is now handled in the writeBinary function
            // it = entries.erase(it);
    
            dm->addMetaData(module.metadata);
            dm->addData(module.data);
    
            // Ensure at end of file
            fileStream.seekp(0, std::ios::end);

            streampos moduleStart = fileStream.tellp();
    
            std::stringstream moduleBuffer;
            dm->writeBinary(moduleStart, moduleBuffer, xrefTable, this->author);

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

Result Writer::cancelThenClose() {

    // Check if file stream is open
    if (!fileStream.is_open()) {
        return Result{false, "No file is open"};
    }

    removeTempFile();
    resetWriter();
    releaseFileLock();
    return Result{true, "File closed successfully"};

}

Result Writer::closeFile() {

    // Check if file stream is open
    if (!fileStream.is_open()) {
        return Result{false, "No file is open"};
    }

    // Check XrefTable is not empty
    if (xrefTable.getEntries().empty()) {
        // No modules to write so delete temp file
        removeTempFile();
        resetWriter();
        return Result{true, "File closed successfully"};
    }

    // Check temp file exists
    if (!std::filesystem::exists(tempFilePath)) {
        return Result{false, "Temp file does not exist"};
    }

    // Check temp file is not empty
    if (std::filesystem::is_empty(tempFilePath)) {
        removeTempFile();
        return Result{false, "Empty temp file, so removed"};
    }

    streampos moduleGraphOffset = fileStream.tellp();

    // Write module graph to temp file
    try {
        uint32_t moduleGraphSize = moduleGraph.writeModuleGraph(fileStream);

        // Add ModuleGraph offset to XREF table
        xrefTable.setModuleGraphOffset(moduleGraphOffset);
        xrefTable.setModuleGraphSize(moduleGraphSize);

    } catch (const std::exception& e) {
        fileStream.close();
        removeTempFile();
        return Result{false, "Exception writing module graph: " + std::string(e.what())};
    }

    cout << "Displaying encounters" << endl << endl;
    moduleGraph.displayEncounters();

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
        removeTempFile();
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
        removeTempFile();
        return Result{false, "Exception renaming temp file: " + std::string(e.what())};
    }

    // Reset writer
    newFile = false;

    releaseFileLock();
    return Result{true, "File closed successfully"};
}

void Writer::resetWriter() {
    fileStream.close();
    header = Header();
    xrefTable.clear();
    tempFilePath.clear();
    filePath.clear();
    moduleGraph = ModuleGraph();
    // author.clear();
}

Result Writer::setUpFileStream(std::string& filename) {

    resetWriter();

    // Acquire a lock on the main file
    try {
        fileLock = std::make_unique<boost::interprocess::file_lock>(filename.c_str());
        if (!fileLock->try_lock()) {
            return Result{false, "File is already locked by another process"};
        }
    } catch (const std::exception& e) {
        return Result{false, "Failed to acquire file lock: " + std::string(e.what())};
    }

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

    return Result{true, "File stream set up successfully"};
}


void Writer::printEncounterPath(const UUID& encounterId) {
    moduleGraph.printEncounterPath(encounterId);
}

/* ======================================================= */
/* =============== MODULE GRAPH OPERATIONS =============== */
/* ======================================================= */


std::expected<UUID, std::string> Writer::createNewEncounter() {

    // Check if file stream is open
    if (!fileStream.is_open()) {
        return std::unexpected("No file is open");
    }
    
    return moduleGraph.createEncounter();
}

std::expected<UUID, std::string> Writer::addModuleToEncounter(
    const UUID& encounterId, const std::string& schemaPath, const ModuleData& module) {

    // Check if file stream is open
    if (!fileStream.is_open()) {
        return std::unexpected("No file is open");
    }

    // Check if encounter exists
    if (!moduleGraph.encounterExists(encounterId)) {

        const auto& encounters = moduleGraph.getEncounters();
        for (const auto& [encounterId, encounter] : encounters) {
            cout << "Encounter " << encounterId.toString() << ": " << encounter.rootModule.value().toString() << " -> " << encounter.lastModule.value().toString() << endl;
        }

        return std::unexpected("Encounter ID " + encounterId.toString() + " not found");
    }
    
    UUID moduleId = UUID();

    try {
        moduleGraph.addModuleToEncounter(encounterId, moduleId);
    } catch (const std::exception& e) {
        return std::unexpected("Exception adding module to encounter: " + std::string(e.what()));
    }

    try {
        auto result = addModule(schemaPath, moduleId, module);
        if (!result.success) {
            moduleGraph.removeModuleFromEncounter(encounterId, moduleId);
            return std::unexpected(result.message);
        }
    } catch (const std::exception& e) {
        moduleGraph.removeModuleFromEncounter(encounterId, moduleId);
        return std::unexpected("Exception adding module to encounter: " + std::string(e.what()));
    }

    return moduleId;
}

std::expected<UUID, std::string> Writer::addVariantModule(
    const UUID& parentModuleId, const std::string& schemaPath, const ModuleData& module) {


        // Check if file stream is open
        if (!fileStream.is_open()) {
            return std::unexpected("No file is open");
        }
        
        // Check if parent module exists
        if (!xrefTable.contains(parentModuleId)) {
            return std::unexpected("Parent module does not exist");
        }

        UUID moduleId = UUID();

        try {
            moduleGraph.addModuleLink(moduleId, parentModuleId, ModuleLinkType::VARIANT_OF);
        } catch (const std::exception& e) {
            return std::unexpected("Exception adding derived module: " + std::string(e.what()));
        }

        auto result = addModule(schemaPath, moduleId, module);
        
        if (!result.success) {
            moduleGraph.removeModuleLink(parentModuleId, moduleId, ModuleLinkType::VARIANT_OF);
            return std::unexpected(result.message);
        }

        return moduleId;
}


std::expected<UUID, std::string> Writer::addAnnotation(
    const UUID& parentModuleId, const std::string& schemaPath, const ModuleData& module) {

        // Check if file stream is open
        if (!fileStream.is_open()) {
            return std::unexpected("No file is open");
        }
            
        // Check if parent module exists
        if (!xrefTable.contains(parentModuleId)) {
            return std::unexpected("Parent module does not exist");
        }

        UUID moduleId = UUID();

        try {
            moduleGraph.addModuleLink(moduleId, parentModuleId, ModuleLinkType::ANNOTATES);
        } catch (const std::exception& e) {
            return std::unexpected("Exception adding annotation: " + std::string(e.what()));
        }

        auto result = addModule(schemaPath, moduleId, module);
        
        if (!result.success) {
            moduleGraph.removeModuleLink(parentModuleId, moduleId, ModuleLinkType::ANNOTATES);
            return std::unexpected(result.message);
        }

        return moduleId;

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

Result Writer::writeModule(
    std::ostream& outfile, const std::string& schemaPath, 
    UUID moduleId,
    const ModuleData& moduleData, EncryptionData encryptionData) {

    // Load schema from file
    std::ifstream schemaFile(schemaPath);
    if (!schemaFile.is_open()) {
        cerr << "Failed to open schema file: " << schemaPath << endl;
        return Result{false, "Failed to open schema file"};
    }
    
    nlohmann::json schemaJson;
    schemaFile >> schemaJson;
    
    string moduleType = schemaJson["module_type"];
    ModuleType type = module_type_from_string(moduleType);

    unique_ptr<DataModule> dm;
    
    // CREATE MODULE
    switch (type) {
        case ModuleType::Image: {
            dm = make_unique<ImageData>(schemaPath, schemaJson, moduleId, encryptionData);
            break;
        }
        case ModuleType::Tabular: {
            dm = make_unique<TabularData>(schemaPath, schemaJson, moduleId, encryptionData);
            break;
        }
        default:

            return Result{false, "Unknown module type: " + moduleType};
    }

    // ADD DATA TO MODULE
    dm->addMetaData(moduleData.metadata);
    dm->addData(moduleData.data);

    // Ensure at end of file
    outfile.seekp(0, std::ios::end);

    streampos moduleStart = outfile.tellp();

    // Reset ZSTD statistics for this module
    ZstdCompressor::resetStatistics();

    // WRITE MODULE TO FILE
    std::stringstream moduleBuffer;
    dm->writeBinary(moduleStart, moduleBuffer, xrefTable, this->author);

    string bufferData = moduleBuffer.str();
    outfile.write(reinterpret_cast<char*>(bufferData.data()), bufferData.size());

    // Print ZSTD compression summary for this module
    std::cout << "Module ZSTD compression summary:" << std::endl;
    ZstdCompressor::printSummary();

    return Result{true, "Module written successfully"};
}

void Writer::removeTempFile() {
    if (std::filesystem::exists(tempFilePath)) {
        std::filesystem::remove(tempFilePath);
    }
    if (std::filesystem::exists(filePath) && std::filesystem::is_empty(filePath)) {
    std::filesystem::remove(filePath);
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

void Writer::releaseFileLock() { 
    if (fileLock) {
        fileLock->unlock();
        fileLock.reset();
    }
}

Writer::~Writer() {
    if (fileLock && fileLock->try_lock()) {
        fileLock->unlock();
    }
}