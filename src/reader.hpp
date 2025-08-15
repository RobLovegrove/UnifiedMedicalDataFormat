#ifndef READER_HPP
#define READER_HPP

#include <string>
#include <nlohmann/json.hpp>
#include <fstream>

#include "Header/header.hpp"
#include "Xref/xref.hpp"
#include "DataModule/dataModule.hpp"
#include <expected>

class Reader {
protected: 
    static constexpr size_t MAX_IN_MEMORY_MODULE_SIZE = 2 << 20; // 1 << 20 is bitwise 1 * 2^20 = 1,048,576 (1 megabyte) 
    
    std::ifstream inFile;

    Header header = Header();
    XRefTable xrefTable;
    std::vector<std::unique_ptr<DataModule>> loadedModules;

public:
    // Returns true on success (patientData filled), false on failure.
    bool readFile(const std::string& filename);
    void loadModule(const XrefEntry& entry);
    
    // New methods for Python integration
    bool openFile(const std::string& filename);
    void closeFile();

    nlohmann::json getFileInfo();
    nlohmann::json getModuleList();
    std::expected<ModuleData, std::string> getModuleData(const std::string& moduleId);
    
    // Getter for loaded modules (for pybind11 access)
    const std::vector<std::unique_ptr<DataModule>>& getLoadedModules() const { return loadedModules; }

};

#endif