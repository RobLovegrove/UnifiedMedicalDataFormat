#include "umdfFile.hpp"
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

bool UMDFFile::openFile(const std::string& filename) {

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

void UMDFFile::closeFile() {
    if (fileStream.is_open()) {

        xrefTable.clear();
        loadedModules.clear();

        fileStream.close();
    }
}

/* ================================================== */
/* =============== READING OPERATIONS =============== */
/* ================================================== */


nlohmann::json UMDFFile::getFileInfo() {
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

std::expected<ModuleData, std::string> UMDFFile::getModuleData(
    const std::string& moduleId) {

    if (!fileStream.is_open()) {
        return std::unexpected("No file is currently open");  // Error case
    }
    
    // Find the module in the loadedModules vector
    for (const auto& module : loadedModules) {
        if (module->getModuleID().toString() == moduleId) {
            return module->getDataWithSchema();  // Success case
        }
    }

    cout << "Module not found in loadedModules, loading from file" << endl;
    
    // If the module is not found, load it from the file
    for (const auto& entry : xrefTable.getEntries()) {
        if (entry.id.toString() == moduleId) {
            loadModule(entry);
            cout << "Module loaded" << endl;
            return loadedModules.back()->getDataWithSchema();
        }
    }

    return std::unexpected("Module not found: " + moduleId);
}

void UMDFFile::loadModule(const XrefEntry& entry) {

     if (entry.size <= MAX_IN_MEMORY_MODULE_SIZE) {
        vector<char> buffer(entry.size);
        fileStream.seekg(entry.offset);
        fileStream.read(buffer.data(), entry.size);
        istringstream stream(string(buffer.begin(), buffer.end()));

        cout << "Reading module: " << entry.id.toString() << endl;
        unique_ptr<DataModule> dm = DataModule::fromStream(stream, entry.offset, entry.type);

        if (!dm) {
            cout << "Skipped unknown or unsupported module type: " << entry.type << endl;
            return;
        }
        loadedModules.push_back(std::move(dm));
    
    }
    else {
        //unique_ptr<DataModule> dm = DataModule::fromFile(inFile, entry.offset);
        cout << "TODO: Handle a large DataModule" << endl;
    }
}

/* ================================================== */
/* =============== WRITING OPERATIONS =============== */
/* ================================================== */

// struct ModuleData {
//     nlohmann::json metadata;
//    std::variant<nlohmann::json, std::vector<uint8_t>, std::vector<ModuleData>> data;
// };

std::expected<std::vector<UUID>, std::string> UMDFFile::writeNewFile(std::string& filename, 
    std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas) {

    std::expected<std::vector<UUID>, std::string> result;

    std::vector<UUID> moduleIds;

    cout << "Writing new file: " << filename << endl;
    
    // OPEN FILE
    fileStream.open(filename, std::ios::out | std::ios::binary);
    if (!fileStream.is_open()) return std::unexpected("Failed to open file");

    cout << "File opened" << endl;

    // CREATE HEADER
    Header header;
    if (!header.writePrimaryHeader(fileStream)) return std::unexpected("Failed to write header");

    cout << "Header written" << endl;

    // Write Modules to file
    result = writeModules(modulesWithSchemas);
    if (!result) return std::unexpected(result.error());
    moduleIds = result.value();

    cout << "Modules written" << endl;
    
    // WRITE XREF TABLE at the end
    if (!writeXref(fileStream)) return std::unexpected("Failed to write xref table");

    // CLOSE FILE
    closeFile();
    return moduleIds;
}



std::expected<std::vector<UUID>, std::string> UMDFFile::addModules(std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas) {

    std::expected<std::vector<UUID>, std::string> result;

    // Check if file is open
    if (!fileStream.is_open()) return std::unexpected("No file is currently open");

    // Check if XREF table is loaded
    if (xrefTable.getEntries().size() == 0) {
        xrefTable = XRefTable::loadXrefTable(fileStream);
    }

    // Write Modules to file
    result = writeModules(modulesWithSchemas);
    if (!result) return std::unexpected(result.error());
    std::vector<UUID> moduleIds = result.value();

    // Make old XREF table obsolete (now with improved stream handling)
    xrefTable.setObsolete(fileStream);

    // WRITE new XREF TABLE at the end
    if (!writeXref(fileStream)) return std::unexpected("Failed to write xref table");

    // CLOSE FILE
    closeFile();
    return moduleIds;
}

bool UMDFFile::updateModules(
    std::vector<std::pair<std::string, ModuleData>>& moduleUpdates) {

    // Check if file is open
    if (!fileStream.is_open()) return false;

    // Check if XREF table is loaded
    if (xrefTable.getEntries().size() == 0) {
        xrefTable = XRefTable::loadXrefTable(fileStream);
    }

    for (const auto& [moduleId, moduleData] : moduleUpdates) {

        // For testing purposes print the metadata of the current module

        auto data = getModuleData(moduleId);
        cout << "Metadata of the current module: " << data.value().metadata << endl;


        auto& entries = xrefTable.getEntries();  // Get reference to avoid copying
        for (auto it = entries.begin(); it != entries.end(); ) {
            if (it->id.toString() == moduleId) {
                // Go to module offset in file
                fileStream.seekg(it->offset);
        
                // Read the DataHeader
                DataHeader dataHeader;
                dataHeader.readDataHeader(fileStream);
        
                // Return to the start of the module
                fileStream.seekg(it->offset);
        
                // Update the module's isCurrent flag to false
                dataHeader.updateIsCurrent(false, fileStream);
        
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
        
                // SAFELY delete the old module from the xref table
                it = entries.erase(it);
        
                dm->addMetaData(moduleData.metadata);
                dm->addData(moduleData.data);
        
                // Ensure at end of file
                fileStream.seekp(0, std::ios::end);
        
                dm->writeBinary(fileStream, xrefTable);

                // Update loadedModules with the new module
                for (auto moduleIt = loadedModules.begin(); moduleIt != loadedModules.end(); ) {
                    if ((*moduleIt)->getModuleID().toString() == moduleId) {
                        moduleIt = loadedModules.erase(moduleIt);
                        loadedModules.push_back(std::move(dm));
                        break;
                    }
                    ++moduleIt;
                }
                
                // Since we found and processed the module, we can break
                break;
            } else {
                ++it;  // Move to next element
            }
        }

        data = getModuleData(moduleId);
        cout << "Metadata of the current module: " << data.value().metadata << endl;
    }

    // Make old XREF table obsolete (now with improved stream handling)
    xrefTable.setObsolete(fileStream);

    // WRITE new XREF TABLE at the end
    if (!writeXref(fileStream)) return false;

    // CLOSE FILE
    closeFile();
    return true;
}


// Helper functions
bool UMDFFile::writeXref(std::ostream& outfile) { 
    // Explicitly seek to end of file
    outfile.seekp(0, std::ios::end);
    
    uint64_t offset = outfile.tellp();  // Get position at end
    xrefTable.setXrefOffset(offset);
    return xrefTable.writeXref(outfile);
}

std::expected<std::vector<UUID>, std::string> UMDFFile::writeModules(
    const std::vector<std::pair<std::string, ModuleData>>& modulesWithSchemas) {
        
    std::vector<UUID> moduleIds;  // Declare the variable here
        
    // CREATE MODULES 
    for (const auto& [schemaPath, moduleData] : modulesWithSchemas) {

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
        fileStream.seekp(0, std::ios::end);

        // WRITE MODULE TO FILE
        dm->writeBinary(fileStream, xrefTable);

        moduleIds.push_back(dm->getModuleID());
    }

    return moduleIds;
}