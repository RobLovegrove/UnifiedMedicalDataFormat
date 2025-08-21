#include "reader.hpp"
#include "Utility/Compression/ZstdCompressor.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <expected>
#include <nlohmann/json.hpp>
#include <optional>

using namespace std;

bool Reader::openFile(const std::string& filename) {

    if (fileStream.is_open()) {
        closeFile();
    }

    // UMDFFile opens the stream
    fileStream.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!fileStream.is_open()) return false;
    
    // Read header and confirm UMDF
    if (!header.readPrimaryHeader(fileStream)) { 
        closeFile();
        return false; 
    } 

    return true;

}

void Reader::closeFile() {
    if (fileStream.is_open()) {

        xrefTable.clear();
        loadedModules.clear();

        fileStream.close();
    }
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

    cout << "Module not found in loadedModules, loading from file" << endl;
    
    // If the module is not found, load it from the file
    for (const auto& entry : xrefTable.getEntries()) {
        if (entry.id.toString() == moduleId) {
            auto error = loadModule(entry);
            if (!error) {
                cout << "Module loaded" << endl;
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

        cout << "Reading module: " << entry.id.toString() << endl;

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
        cout << "TODO: Handle a large DataModule" << endl;
        return "TODO: Handle a large DataModule";
    }

    return nullopt;
}