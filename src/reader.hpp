#ifndef READER_HPP
#define READER_HPP

#include <memory>
#include <string>
#include <fstream>
#include <expected>
#include <vector>
#include <optional>
#include "Header/header.hpp"
#include "Xref/xref.hpp"
#include "DataModule/dataModule.hpp"
#include "DataModule/ModuleData.hpp"
#include "Utility/uuid.hpp"
#include "./writer.hpp"

class Reader {
private:
    Header header;
    XRefTable xrefTable;
    ModuleGraph moduleGraph;

    static constexpr size_t MAX_IN_MEMORY_MODULE_SIZE = 100 * 1024 * 1024;  // 100MB

    std::ifstream fileStream;
    std::vector<std::unique_ptr<DataModule>> loadedModules;

    bool isOpen() const { return fileStream.is_open(); }

public:

    void printEncounterPath(const UUID& encounterId);

    // Reading operations 
    nlohmann::json getFileInfo();
    std::expected<ModuleData, std::string> getModuleData(const std::string& moduleId);
    

    std::optional<std::string> loadModule(const XrefEntry& entry);

    // File management
    Result openFile(const std::string& filename);
    Result closeFile();

};


#endif