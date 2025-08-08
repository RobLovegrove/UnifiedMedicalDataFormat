#ifndef READER_HPP
#define READER_HPP

#include <string>
#include <nlohmann/json.hpp>

#include "Header/header.hpp"
#include "Xref/xref.hpp"
#include "DataModule/dataModule.hpp"

class Reader {
private: 
    Header header = Header();
    XRefTable xrefTable;
    std::vector<std::unique_ptr<DataModule>> loadedModules;

public:
    // Returns true on success (patientData filled), false on failure.
    bool readFile(const std::string& filename);
    
    // New methods for Python integration
    nlohmann::json getFileInfo() const;
    nlohmann::json getModuleList() const;
    nlohmann::json getModuleData(const std::string& moduleId) const;
    std::vector<std::string> getModuleIds() const;

};

#endif