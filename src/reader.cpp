#include "reader.hpp"
#include "Header/header.hpp"
#include "Utility/utils.hpp"
#include "Xref/xref.hpp"
#include "DataModule/Header/dataHeader.hpp"
#include "DataModule/Tabular/tabularData.hpp"
#include "DataModule/Image/imageData.hpp"

#include <fstream>
#include <cstdint>
#include <vector>
#include <iostream>
#include <string>
#include <sstream> 
#include <expected>

using namespace std;

// bool Reader::readFile(const string& filename) { 
    
//     inFile.open(filename, ios::binary);
//     if (!inFile) return false;
    
//     // Read header and confirm UMDF
//     if (!header.readPrimaryHeader(inFile)) { return false; } 

//     // load Xref Table
//     xrefTable = XRefTable::loadXrefTable(inFile);

//     cout << "XRefTable successfully read" << endl;
//     cout << xrefTable << endl; // 1 << 20 is bitwise 1 * 2^20 = 1,048,576 (1 megabyte) 

//     // Clear any previously loaded modules
//     loadedModules.clear();

//     // Iterate through XrefTable reading data
//     for (const XrefEntry& entry : xrefTable.getEntries()) {
//         cout << "Loading module: " << entry.id.toString() << endl;
//         loadModule(entry);
//     }
//     return true;
// }

// void Reader::loadModule(const XrefEntry& entry) {

//     if (entry.type == static_cast<uint8_t>(ModuleType::FileHeader)) {
//         // Skip file header entries
//         cout << "Skipping file header entry" << endl;
//         return; 
//     }
//     else if (entry.size <= MAX_IN_MEMORY_MODULE_SIZE) {
//         vector<char> buffer(entry.size);
//         externalStream->seekg(entry.offset);
//         externalStream->read(buffer.data(), entry.size);
//         istringstream stream(string(buffer.begin(), buffer.end()));

//         cout << "Reading module: " << entry.id.toString() << endl;
//         unique_ptr<DataModule> dm = DataModule::fromStream(stream, entry.offset, entry.type);

//         if (!dm) {
//             cout << "Skipped unknown or unsupported module type: " << entry.type << endl;
//             return;
//         }

//         dm->printMetadata(cout);
//         dm->printData(cout);

//         loadedModules.push_back(std::move(dm));
    
//     }
//     else {
//         //unique_ptr<DataModule> dm = DataModule::fromFile(inFile, entry.offset);
//         cout << "TODO: Handle a large DataModule" << endl;
//     }
// }


// bool Reader::verifyFile(std::istream& fileStream) {
//     // Store reference to external stream
//     externalStream = &fileStream;
    
//     // Read header and confirm UMDF
//     if (!header.readPrimaryHeader(fileStream)) { return false; } 

//     xrefTable.clear();
//     loadedModules.clear();

//     return true;
// }

// nlohmann::json Reader::getFileInfo() {

//     nlohmann::json result;

//     if (externalStream == nullptr) {
//         result["success"] = false;
//         result["error"] = "No file is currently open";
//         return result;
//     }

//     if (xrefTable.getEntries().size() == 0) {
//         xrefTable = XRefTable::loadXrefTable(*externalStream);
//     }
//     result["success"] = true;
//     result["module_count"] = xrefTable.getEntries().size() - 1; // -1 to exclude the file header entry
    
//     nlohmann::json moduleList = nlohmann::json::array();
//     for (const auto& entry : xrefTable.getEntries()) {
//         nlohmann::json moduleInfo;
//         if (entry.type == static_cast<uint8_t>(ModuleType::FileHeader)) {
//             continue;
//         }
//         else {
//             ModuleType type = static_cast<ModuleType>(entry.type);
//             moduleInfo["type"] = module_type_to_string(type);
//             moduleInfo["uuid"] = entry.id.toString();
//             moduleList.push_back(moduleInfo);
//         }
//     }
//     result["modules"] = moduleList;
//     return result;
// }

// Copied over for quick reference
// struct ModuleData {
//     nlohmann::json metadata;
//     std::variant<
//         nlohmann::json,                    // For tabular data
//         std::vector<uint8_t>,              // For frame pixel data
//         std::vector<ModuleData>            // For N-dimensional data
//     > data;
// };

// std::expected<ModuleData, std::string> Reader::getModuleData(const std::string& moduleId) {

//     if (externalStream == nullptr) {
//         return std::unexpected("No file is currently open");  // Error case
//     }
    
//     // Find the module in the loadedModules vector
//     for (const auto& module : loadedModules) {
//         if (module->getModuleID().toString() == moduleId) {
//             return module->getDataWithSchema();  // Success case
//         }
//     }
    
//     // If the module is not found, load it from the file
//     for (const auto& entry : xrefTable.getEntries()) {
//         if (entry.id.toString() == moduleId) {
//             loadModule(entry);
//             return loadedModules.back()->getDataWithSchema();
//         }
//     }

//     return std::unexpected("Module not found: " + moduleId);
// }

// void Reader::closeFile() {
//     if (externalStream != nullptr) {
//         externalStream->close();
//         xrefTable.clear();
//         loadedModules.clear();
//     }
// }