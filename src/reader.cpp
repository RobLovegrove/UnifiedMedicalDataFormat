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

using namespace std;

bool Reader::readFile(const string& filename) { 
    
    ifstream inFile(filename, ios::binary);
    if (!inFile) return false;
    
    // Read header and confirm UMDF
    if (!header.readPrimaryHeader(inFile)) { return false; } 

    // load Xref Table
    xrefTable = XRefTable::loadXrefTable(inFile);

    cout << "XRefTable successfully read" << endl;
    cout << xrefTable << endl;

    constexpr size_t MAX_IN_MEMORY_MODULE_SIZE = 1 << 20; // 1 << 20 is bitwise 1 * 2^20 = 1,048,576 (1 megabyte) 

    // Clear any previously loaded modules
    loadedModules.clear();

    // Iterate through XrefTable reading data
    for (const XrefEntry& entry : xrefTable.getEntries()) {
        if (entry.type == static_cast<uint8_t>(ModuleType::FileHeader)) {
            // Skip file header entries
            continue;
        }
        else if (entry.size <= MAX_IN_MEMORY_MODULE_SIZE) {
            vector<char> buffer(entry.size);
            inFile.seekg(entry.offset);
            inFile.read(buffer.data(), entry.size);
            istringstream stream(string(buffer.begin(), buffer.end()));
            unique_ptr<DataModule> dm = DataModule::fromStream(stream, entry.offset, entry.type);
            if (!dm) {
                cout << "Skipped unknown or unsupported module type: " << entry.type << endl;
                continue;
            }
            
            // Store the module instead of just printing
            loadedModules.push_back(std::move(dm));
            
            // Print structured data using getModuleData for better output
            auto moduleData = loadedModules.back()->getDataWithSchema();
            cout << "Module ID: " << loadedModules.back()->getModuleID().toString() << endl;
            cout << "Schema: " << loadedModules.back()->getSchema()["$id"] << endl;
            cout << "Metadata: " << moduleData.metadata.dump(2) << endl;
            
            // Handle the variant data
            std::visit([&](const auto& data) {
                using T = std::decay_t<decltype(data)>;
                if constexpr (std::is_same_v<T, nlohmann::json>) {
                    cout << "Data: " << data.dump(2) << endl;
                } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                    cout << "Data: " << data.size() << " bytes of pixel data" << endl;
                } else if constexpr (std::is_same_v<T, std::vector<ModuleData>>) {
                    cout << "Data: " << data.size() << " frames:" << endl;
                    for (size_t i = 0; i < data.size(); ++i) {
                        cout << "  Frame " << i << ":" << endl;
                        cout << "    Metadata: " << data[i].metadata.dump(2) << endl;
                        // Handle the nested variant
                        std::visit([&](const auto& d) {
                            using D = std::decay_t<decltype(d)>;
                            if constexpr (std::is_same_v<D, nlohmann::json>) {
                                cout << "    Data: " << d.dump(2) << endl;
                            } else if constexpr (std::is_same_v<D, std::vector<uint8_t>>) {
                                cout << "    Data: " << d.size() << " bytes of pixel data" << endl;
                            } else if constexpr (std::is_same_v<D, std::vector<ModuleData>>) {
                                cout << "    Data: nested data structure" << endl;
                            }
                        }, data[i].data);
                    }
                }
            }, moduleData.data);
            cout << endl;
        }
        else {
            //unique_ptr<DataModule> dm = DataModule::fromFile(inFile, entry.offset);
            cout << "TODO: Handle a large DataModule" << endl;
        }
    }
    return true;
}

// Methods for Python integration
nlohmann::json Reader::getFileInfo() const {
    nlohmann::json info;
    info["module_count"] = loadedModules.size();
    info["xref_entries"] = xrefTable.getEntries().size();
    return info;
}

nlohmann::json Reader::getModuleList() const {
    nlohmann::json moduleList = nlohmann::json::array();
    
    for (size_t i = 0; i < loadedModules.size(); ++i) {
        nlohmann::json moduleInfo;
        moduleInfo["index"] = i;
        moduleInfo["type"] = static_cast<int>(loadedModules[i]->getModuleType());
        moduleInfo["uuid"] = loadedModules[i]->getModuleID().toString();
        moduleList.push_back(moduleInfo);
    }
    
    return moduleList;
}

nlohmann::json Reader::getModuleData(const std::string& moduleId) const {
    // Find module by UUID or index
    for (const auto& module : loadedModules) {
        if (module->getModuleID().toString() == moduleId) {
            // Get the ModuleData structure
            auto moduleData = module->getDataWithSchema();
            
            // Convert ModuleData to JSON for Python compatibility
            nlohmann::json result;
            result["module_id"] = module->getModuleID().toString();
            result["schema_url"] = module->getSchema()["$id"];
            result["metadata"] = moduleData.metadata;
            
            // Convert variant to JSON using explicit type handling
            std::visit([&result](const auto& data) {
                using T = std::decay_t<decltype(data)>;
                if constexpr (std::is_same_v<T, nlohmann::json>) {
                    result["data"] = data;
                } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                    result["data"] = data;  // nlohmann::json can handle vectors
                } else if constexpr (std::is_same_v<T, std::vector<ModuleData>>) {
                    // Convert ModuleData array to JSON
                    nlohmann::json dataArray = nlohmann::json::array();
                    for (const auto& md : data) {
                        nlohmann::json mdJson;
                        mdJson["metadata"] = md.metadata;
                        // Handle the nested variant
                        std::visit([&mdJson](const auto& d) {
                            using D = std::decay_t<decltype(d)>;
                            if constexpr (std::is_same_v<D, nlohmann::json>) {
                                mdJson["data"] = d;
                            } else if constexpr (std::is_same_v<D, std::vector<uint8_t>>) {
                                mdJson["data"] = d;
                            } else if constexpr (std::is_same_v<D, std::vector<ModuleData>>) {
                                // Recursive conversion for nested ModuleData arrays
                                nlohmann::json nestedArray = nlohmann::json::array();
                                for (const auto& nestedMd : d) {
                                    nlohmann::json nestedJson;
                                    nestedJson["metadata"] = nestedMd.metadata;
                                    // For now, just store the nested data as-is
                                    nestedJson["data"] = "nested_data";  // Placeholder
                                    nestedArray.push_back(nestedJson);
                                }
                                mdJson["data"] = nestedArray;
                            }
                        }, md.data);
                        dataArray.push_back(mdJson);
                    }
                    result["data"] = dataArray;
                }
            }, moduleData.data);
            
            return result;
        }
    }
    return nlohmann::json();
}

std::vector<std::string> Reader::getModuleIds() const {
    std::vector<std::string> ids;
    for (const auto& module : loadedModules) {
        ids.push_back(module->getModuleID().toString());
    }
    return ids;
}
