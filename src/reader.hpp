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
#include "./AuditTrail/auditTrail.hpp"

class Reader {
private:
    Header header;
    XRefTable xrefTable;

    ModuleGraph moduleGraph;

    std::unique_ptr<AuditTrail> auditTrail;

    static constexpr size_t MAX_IN_MEMORY_MODULE_SIZE = 100 * 1024 * 1024;  // 100MB

    std::ifstream fileStream;
    std::vector<std::unique_ptr<DataModule>> loadedModules;

    bool isOpen() const { return fileStream.is_open(); }

    std::expected<std::unique_ptr<DataModule>, std::string> loadModule
        (uint64_t offset, uint32_t size, ModuleType type);

public:

    void printEncounterPath(const UUID& encounterId);

    std::expected<ModuleData, std::string> getModuleData(const std::string& moduleId);
    
    std::expected<std::vector<ModuleTrail>, std::string> getAuditTrail(const UUID& moduleId);
    std::expected<ModuleData, std::string> getAuditData(const ModuleTrail& module);
    
    // File management
    Result openFile(const std::string& filename, std::string password = "");
    Result closeFile();

    // Reading operations 
    nlohmann::json getFileInfo();

};


#endif