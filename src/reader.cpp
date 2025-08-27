#include "reader.hpp"
#include "writer.hpp"
#include "Utility/Compression/ZstdCompressor.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <expected>
#include <nlohmann/json.hpp>
#include <optional>

using namespace std;

Result Reader::openFile(const std::string& filename, std::string password) {

    if (fileStream.is_open()) {
        closeFile();
    }

    // Reset header
    header = Header();
    xrefTable = XRefTable();
    loadedModules.clear();

    // UMDFFile opens the stream
    fileStream.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!fileStream.is_open()) return Result{false, "Failed to open file"};
    
    // Read header and confirm UMDF
    auto headerResult = header.readPrimaryHeader(fileStream);
    if (!headerResult) { 
        closeFile();
        return Result{false, "Failed to read header: " + headerResult.error()}; 
    } 

    if (header.getEncryptionData().encryptionType != EncryptionType::NONE) {
        if (password == "") {
            return Result{false, "File is encrypted but no password provided"};
        }
        else {
            header.setEncryptionPassword(password);
        }
    }

    try {
        // Read XREF table
        xrefTable = XRefTable::loadXrefTable(fileStream);
    }
    catch (const std::exception& e) {
        closeFile();
        return Result{false, "Failed to read XREF table: " + string(e.what())};
    }

    try {
        // Read ModuleGraph
        fileStream.seekg(xrefTable.getModuleGraphOffset());

        std::stringstream buffer;

        // allocate temporary space
        std::vector<char> temp(xrefTable.getModuleGraphSize());

        // read raw bytes
        fileStream.read(temp.data(), temp.size());

        // copy into stringstream
        buffer.write(temp.data(), temp.size());

        // Read the module graph from the buffer
        moduleGraph = ModuleGraph::readModuleGraph(buffer);

        cout << "Displaying encounters" << endl << endl;
        moduleGraph.displayEncounters();
    }
    catch (const std::exception& e) {
        closeFile();
        return Result{false, "Failed to read ModuleGraph: " + string(e.what())};
    }



    return Result{true, "File opened successfully"};

}

Result Reader::closeFile() {
    if (fileStream.is_open()) {

        xrefTable.clear();
        loadedModules.clear();

        fileStream.close();
    }

    return Result{true, "File closed successfully"};
}

/* ================================================== */
/* =============== READING OPERATIONS =============== */
/* ================================================== */

nlohmann::json Reader::getFileInfo() {
    nlohmann::json result;

    if (!fileStream.is_open()) {
        result["success"] = false;
        result["error"] = "No file is currently open";
        return result;
    }

    if (xrefTable.getEntries().size() == 0) {
        xrefTable = XRefTable::loadXrefTable(fileStream);
    }

    cout << xrefTable << endl;

    result["success"] = true;
    result["module_count"] = xrefTable.getEntries().size();
    
    nlohmann::json moduleList = nlohmann::json::array();
    for (const auto& entry : xrefTable.getEntries()) {
        nlohmann::json moduleInfo;
        ModuleType type = static_cast<ModuleType>(entry.type);
        moduleInfo["type"] = module_type_to_string(type);
        moduleInfo["uuid"] = entry.id.toString();
        moduleList.push_back(moduleInfo);
    }
    result["modules"] = moduleList;

    cout << "Printing encounter paths" << endl;
    const auto& encounters = moduleGraph.getEncounters();
    for (const auto& [encounterId, encounter] : encounters) {
        moduleGraph.printEncounterPath(encounterId);
    }

    return result;
}

std::expected<ModuleData, std::string> Reader::getModuleData(
    const std::string& moduleId) {

    if (!fileStream.is_open()) {
        return std::unexpected("No file is currently open");  // Error case
    }
    
    // Find the module in the loadedModules vector
    for (const auto& module : loadedModules) {
        if (module->getModuleID().toString() == moduleId) {
            return module->getModuleData();  // Success case
        }
    }

    // If the module is not found, load it from the file
    for (const auto& entry : xrefTable.getEntries()) {
        if (entry.id.toString() == moduleId) {
            auto error = loadModule(entry);
            if (!error) {
                return loadedModules.back()->getModuleData();
            }
            else {
                cout << "Error loading module: " << error.value() << endl;
                return std::unexpected("Error loading module: " + error.value());
            }
        }
    }

    return std::unexpected("Module not found: " + moduleId);
}

std::optional<std::string> Reader::loadModule(const XrefEntry& entry) {

     if (entry.size <= MAX_IN_MEMORY_MODULE_SIZE) {
        vector<char> buffer(entry.size);
        fileStream.seekg(entry.offset);
        fileStream.read(buffer.data(), entry.size);
        istringstream stream(string(buffer.begin(), buffer.end()));



        // Start ZSTD summary mode for this module
        ZstdCompressor::startSummaryMode();

        unique_ptr<DataModule> dm;
        try {
            dm = DataModule::fromStream(stream, entry.offset, entry.type, header.getEncryptionData());
            if (!dm) {
                return "Skipped unknown or unsupported module type: " + to_string(entry.type);
            }

            // Validate the module before storing it
            try {
                dm->getModuleData();  // Test if data access works
            } catch (const std::exception& e) {
                return "Module validation failed: " + string(e.what());
            }
        }
        catch (const std::exception& e) {
            return "Error reading module: " + string(e.what());
        }
        
        // Print ZSTD decompression summary for this module and stop summary mode
        if (ZstdCompressor::isSummaryMode()) {
            std::cout << "Module ZSTD decompression summary:" << std::endl;
            ZstdCompressor::printSummary();
            ZstdCompressor::stopSummaryMode();
        }
        
        loadedModules.push_back(std::move(dm));
    }
    else {
        //unique_ptr<DataModule> dm = DataModule::fromFile(inFile, entry.offset);

        return "TODO: Handle a large DataModule";
    }

    return nullopt;
}

std::expected<std::vector<ModuleTrail>, std::string> Reader::getAuditTrail(const UUID& moduleId) {

    if (!fileStream.is_open()) {
        return std::unexpected("No file is currently open");
    }

    // Confirm moduleId is in the xrefTable
    if (!xrefTable.contains(moduleId)) {
        return std::unexpected("Module not found in XREF table");
    }

    try {
        auditTrail = std::make_unique<AuditTrail>(moduleId, fileStream, xrefTable);
        return auditTrail->getModuleTrail();
    }
    catch (const std::exception& e) {
        return std::unexpected("Error getting audit trail: " + string(e.what()));
    }
}

void Reader::printEncounterPath(const UUID& encounterId) {
    moduleGraph.printEncounterPath(encounterId);
}