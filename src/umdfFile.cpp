#include "umdfFile.hpp"
#include "Xref/xref.hpp"
#include "DataModule/dataModule.hpp"
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

        cout << "Module loaded" << endl;

        // dm->printMetadata(cout);
        // dm->printData(cout);

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

bool UMDFFile::writeNewFile(const std::string& filename) {
    //return writer->writeNewFile(filename);
}

bool UMDFFile::addModule(const ModuleData& module) {
    //return writer->addModule(fileStream, module);
}

bool UMDFFile::updateModule(const std::string& moduleId, const ModuleData& module) {
    //return writer->updateModule(fileStream, moduleId, module);
}
